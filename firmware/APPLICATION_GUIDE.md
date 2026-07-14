# BLE Mesh Relay LED Dimmer Network

## Overview

This application transforms up to 100 WBZ451 Curiosity boards into a BLE cluster-based relay network for LED dimming control. A cell phone running the Microchip MBD (Microchip Bluetooth Data) app connects to any board in the network and can send dimmer commands to any other board through multi-hop relay using a cluster hierarchy.

All boards run **the same firmware binary**. Node IDs are assigned wirelessly from the phone using the built-in provisioning system — no per-board recompilation needed.

## Architecture

The network is organized into **10 clusters of up to 10 nodes each**, with designated gateway nodes forming a backbone:

```
Phone (MBD App)
    |
    | BLE Peripheral Connection (Transparent Service)
    v
Gateway_01 ←——→ Gateway_11 ←——→ Gateway_21 ←——→ ... ←——→ Gateway_91
    ↕                 ↕                ↕                       ↕
Nodes 2-10       Nodes 12-20     Nodes 22-30            Nodes 92-99
(Local)          (Local)         (Local)                (Local)
```

Each board runs in **multi-role BLE mode**:
- **Peripheral role**: Accepts connections from the phone or from local nodes
- **Central role**: Initiates connections to gateways (local→gateway or gateway→gateway)

### Node Roles (Automatic from Node ID)

| Node ID | Role | Cluster | Behavior |
|---------|------|---------|----------|
| 1 | Gateway | 0 | Routes between cluster 0 locals and backbone |
| 2-10 | Local | 0 | Connects to Gateway_01 only |
| 11 | Gateway | 1 | Routes between cluster 1 locals and backbone |
| 12-20 | Local | 1 | Connects to Gateway_11 only |
| 21 | Gateway | 2 | Routes between cluster 2 locals and backbone |
| ... | ... | ... | ... |
| 91 | Gateway | 9 | Routes for cluster 9 |
| 92-99 | Local | 9 | Connects to Gateway_91 only |

**Rule**: The first node in each group of 10 is automatically the gateway (IDs: 1, 11, 21, 31, 41, 51, 61, 71, 81, 91).

## Hardware

- **Board**: WBZ451 Curiosity Board (EV96B94A)
- **MCU**: PIC32CX-BZ2 (WBZ451PE module) — ARM Cortex-M4 + 2.4 GHz BLE transceiver
- **RGB LED (D6)**: Red=PB0, Green=PB3, Blue=PB5 (active-high)
- **UART Debug**: PA5 (TX), PA6 (RX) via on-board MCP2200 USB-UART converter
- **Max BLE connections per device**: 6 (HCI limit)

## Quick Start Guide

### Step 1: Build and Flash

Build the firmware **once** in MPLAB X IDE. Flash the **same binary** to all boards.

No per-board configuration needed at compile time.

### Step 2: Provision Each Board

1. Power up a board — green LED blinks 3x, then advertises as "DIMMER_01" (default)
2. Open MBD app on phone → scan → connect to "DIMMER_01"
3. Select "Transparent UART" mode
4. Send: `00 00 00 05 05 XX` (where XX = desired node ID in hex)
5. Board flashes green, reboots, now advertises as "DIMMER_XX"
6. Repeat for each board with a unique ID

**Provisioning commands:**

| Action | Command |
|--------|---------|
| Assign as Node 1 (Gateway cluster 0) | `00 00 00 05 05 01` |
| Assign as Node 2 (Local cluster 0) | `00 00 00 05 05 02` |
| Assign as Node 5 (Local cluster 0) | `00 00 00 05 05 05` |
| Assign as Node 11 (Gateway cluster 1) | `00 00 00 05 05 0B` |
| Assign as Node 20 (Local cluster 1) | `00 00 00 05 05 14` |
| Assign as Node 99 (Local cluster 9) | `00 00 00 05 05 63` |
| Unprovision (erase ID, revert) | `00 00 00 05 06` |

> **DST=0x00** means "execute on the board I'm directly connected to" — works regardless of the board's current ID.

> **The ID is stored in NVM flash** and persists across power cycles. You only provision once per board.

### Step 3: Control LEDs

Once boards are provisioned and powered, they auto-connect based on cluster rules. Send dimmer commands from the phone:

| Action | Command |
|--------|---------|
| Node 2 → 100% Red | `02 00 00 05 01 64 00 00` |
| Node 15 → 50% Green | `0F 00 00 05 01 00 32 00` |
| ALL nodes → 100% Red | `FF 00 00 05 01 64 00 00` |
| ALL nodes → White 50% | `FF 00 00 05 01 32 32 32` |
| ALL nodes → OFF | `FF 00 00 05 01 00 00 00` |
| Cluster 0 broadcast, 100% Red | `C0 00 00 05 01 64 00 00` |
| Cluster 1 broadcast, 50% Green | `C1 00 00 05 01 00 32 00` |
| Cluster 9 broadcast, OFF | `C9 00 00 05 01 00 00 00` |
| Identify Node 25 | `19 00 00 05 04` |
| Broadcast identify | `FF 00 00 05 04` |

## Network Protocol

### Packet Format

```
| Byte 0   | Byte 1   | Byte 2   | Byte 3 | Byte 4 | Byte 5+     |
|----------|----------|----------|--------|--------|-------------|
| DST_ID   | SRC_ID   | SEQ_NUM  | TTL    | CMD    | PAYLOAD...  |
```

| Field    | Description                                           |
|----------|-------------------------------------------------------|
| DST_ID   | Destination: 0x00=this board, 0x01-0x64=specific node, 0xC0-0xC9=cluster broadcast, 0xFF=global broadcast |
| SRC_ID   | Source Node ID (0x00 = phone)                          |
| SEQ_NUM  | Sequence number for duplicate detection                |
| TTL      | Time To Live, decremented each hop (starts at 5)      |
| CMD      | Command ID                                             |
| PAYLOAD  | Command-specific data                                  |

### Commands

| CMD  | Name          | Payload                    | Description                       |
|------|---------------|----------------------------|-----------------------------------|
| 0x01 | SET_DIMMER    | R(1B) G(1B) B(1B)         | Set LED brightness 0-100% per channel |
| 0x02 | GET_STATUS    | (none)                     | Request current LED state          |
| 0x03 | STATUS_RESP   | R(1B) G(1B) B(1B) Peers(1B)| Response with LED state + peer count |
| 0x04 | IDENTIFY      | (none)                     | Flash all LEDs to identify board   |
| 0x05 | PROVISION     | NodeID(1B)                 | Assign node ID, store in NVM, reboot |
| 0x06 | UNPROVISION   | (none)                     | Erase stored ID, reboot to default |

### Dimmer Values

The R, G, B payload bytes represent percentage (0-100 decimal):
- `00` = 0% (LED off)
- `32` = 50% (half brightness)
- `64` = 100% (full brightness)

Values above 100 (0x64) are clamped to 100%.

### Cluster Broadcast Addresses

Use destination `0xC0 + cluster ID` to execute a command on every node in one
cluster. For example, `C0 00 00 05 01 64 00 00` sets all nodes in cluster 0
(node IDs 1-10) to 100% red. Command fields are hexadecimal bytes, so decimal
100% is written as `64`, not `100`.

> **Important**: Node IDs in commands are hexadecimal! Node 15 = `0F`, Node 20 = `14`, Node 99 = `63`.

### Routing Rules

**Local nodes:**
- Forward all packets to their cluster gateway
- Execute commands addressed to them or broadcast
- Do NOT relay further (leaf nodes)

**Gateway nodes:**
- If destination is in MY cluster → forward to local node connections
- If destination is in ANOTHER cluster → forward along the gateway backbone
- If broadcast → forward to all local nodes + all gateway peers
- Execute commands addressed to them or broadcast

**Duplicate Detection:** 32-entry circular cache of (src_id, seq_num) pairs prevents loops.

**TTL=5:** Worst case path: Local → Gateway → Gateway → Gateway → Gateway → Local = 5 hops.

## Connection Topology

### Connection Budget Per Board (6 max)

**Gateway nodes:**

| Slots | Role       | Purpose                              |
|-------|------------|--------------------------------------|
| 1-2   | Central    | Connections to next gateways in chain |
| 1-3   | Peripheral | Local nodes + phone                   |

**Local nodes:**

| Slots | Role       | Purpose                              |
|-------|------------|--------------------------------------|
| 1     | Central    | Connection to cluster gateway         |
| 1     | Peripheral | Phone connection                      |
| 4     | Unused     | Available for future expansion        |

### Auto-Connection Rules

- **Local nodes** scan for and connect ONLY to their cluster's gateway
- **Gateways** scan for and connect to the next 1-2 gateways in the chain
- Gateways accept incoming connections from their local nodes (peripheral role)
- The phone can connect to ANY node (gateway or local)

### Gateway Backbone Chain

```
Gateway_01 →(central)→ Gateway_11 →(central)→ Gateway_21 →(central)→ ...
```

Each gateway connects as CENTRAL to the next gateway. This creates a linked chain for inter-cluster routing.

## Building

### Prerequisites

- MPLAB X IDE with XC32 compiler v4.60+
- PIC32CX-BZ DFP v1.4.243+
- WBZ451 Curiosity Board(s)

### Build Steps

1. Open project in MPLAB X IDE
2. Build (no special configuration needed)
3. Flash to all boards using the same binary
4. Provision each board via phone (see Quick Start)

**Optional**: Set `-DNODE_ID=N` in compiler macros to change the fallback default ID for unprovisioned boards (default is 1).

### Project Structure

```
src/
├── app.c                          Main application state machine
├── app.h                          Application types and message IDs
├── node_config.h                  Cluster macros (auto-computed from node ID)
├── provisioning.c/h               NVM-based node ID storage and management
├── mesh_routing.c/h               Cluster-aware routing, duplicate detection
├── mesh_conn_mgr.c/h             Role + type aware connection manager
├── led_dimmer.c/h                 Software PWM for RGB LED
├── app_ble/
│   ├── app_ble.c                  BLE stack init (multi-role: central+peripheral)
│   ├── app_ble_callbacks.c        Cluster-aware connection logic
│   └── handlers/
│       ├── app_trspc_handler.c    TRSPC: receives data from peer boards → mesh
│       └── app_trsps_handler.c    TRSPS: receives data from phone/peers → mesh
└── config/wbz451/
    ├── configuration.h            BLE config (advertising, scan, max links=6)
    └── ble/profile_ble/
        ├── ble_trspc/             Transparent Profile Client (central role)
        └── ble_trsps/             Transparent Profile Server (peripheral role)
```

## Provisioning System

### How It Works

1. On first boot, board reads NVM flash at address `0x000FF000`
2. If no valid provisioning data found (magic number mismatch) → **unprovisioned mode**
3. Unprovisioned boards use compile-time default ID (1) and advertise as "DIMMER_01"
4. When a PROVISION command (0x05) is received:
   - Node ID is written to NVM flash
   - LED flashes green to confirm
   - Board performs software reset
5. On subsequent boots, the stored ID is loaded from NVM
6. The board advertises and operates with its provisioned identity

### Provisioning Workflow for 20 Boards

```
For each board (1 through 20):
  1. Power up board
  2. Phone → connect to "DIMMER_01"
  3. Send: 00 00 00 05 05 [ID_in_hex]
  4. Board reboots as DIMMER_[ID]
  5. Disconnect, move to next board
```

Example IDs to assign for a 20-board deployment:
- Board 1: `00 00 00 05 05 01` → Gateway (cluster 0)
- Board 2: `00 00 00 05 05 02` → Local
- Board 3: `00 00 00 05 05 03` → Local
- ...
- Board 10: `00 00 00 05 05 0A` → Local
- Board 11: `00 00 00 05 05 0B` → Gateway (cluster 1)
- Board 12: `00 00 00 05 05 0C` → Local
- ...
- Board 20: `00 00 00 05 05 14` → Local

### Unprovisioning

Send `00 00 00 05 06` to erase the stored ID. The board reboots and returns to unprovisioned state.

## Debug UART Output

Connect a serial terminal to the board's USB port (115200 baud).

**Unprovisioned board:**
```
=== UNPROVISIONED (default ID=1) ===
Send PROVISION cmd to assign ID
Advertising...
Scanning for peers...
```

**Provisioned gateway:**
```
=== DIMMER_01 Started ===
Advertising...
Scanning for peers...
Found DIMMER_11
Connecting to DIMMER_11
Connected hdl=0x0071 role=C type=3
TRSPC disc complete hdl=0x0071
Connected hdl=0x0072 role=P type=0
TRSPS TX ready
HEX parsed 8 B
SET R=100 G=0 B=0
```

**Provisioned local node:**
```
=== DIMMER_05 Started ===
Advertising...
Scanning for peers...
Found DIMMER_01
Connecting to DIMMER_01
Connected hdl=0x0071 role=C type=3
TRSPC disc complete hdl=0x0071
Scan done, peers=1
```

**Provisioning event:**
```
HEX parsed 6 B
PROVISION id=5
Stored. Rebooting...
=== DIMMER_05 Started ===
```

## LED Behavior

| State              | LED Pattern                          |
|--------------------|--------------------------------------|
| Boot               | Green blinks 3 times                 |
| Idle               | All LEDs off                         |
| Dimmer active      | RGB LED at commanded brightness       |
| Identify command   | All LEDs flash rapidly (5 seconds)   |
| Provision success  | Green flash, then reboot             |
| Unprovision        | Red flash, then reboot               |

## Test Scenarios

**1. Single board (provisioning test):**
- Flash board, connect phone, send `00 00 00 05 05 02`
- Verify board reboots as "DIMMER_02"
- Send `02 00 00 05 01 64 00 00` → red LED on

**2. Same-cluster (2 boards):**
- Provision board A as ID 1 (Gateway), board B as ID 2 (Local)
- Board B auto-connects to Board A
- Phone → Board A, send `02 00 00 05 01 00 64 00` → Board B green LED

**3. Cross-cluster (3 boards):**
- Board A: ID 1 (Gateway cluster 0)
- Board B: ID 11 (Gateway cluster 1)
- Board C: ID 12 (Local cluster 1)
- Phone → Board A, send `0C 00 00 05 01 64 00 00` → Board C red LED
- Route: Phone → GW_01 → GW_11 → Node_12

**4. Broadcast:**
- Multiple boards provisioned and connected
- Send `FF 00 00 05 01 32 32 32` → all nodes dim white

## Scaling Guide

| Network Size | Clusters | Gateways | Node IDs |
|-------------|----------|----------|----------|
| 10 devices  | 1        | 1 (ID=1) | 1-10     |
| 20 devices  | 2        | 2 (IDs 1, 11) | 1-20 |
| 50 devices  | 5        | 5 (IDs 1, 11, 21, 31, 41) | 1-50 |
| 100 devices | 10       | 10 (IDs 1, 11, ..., 91) | 1-99 |

## Troubleshooting

| Symptom                     | Cause                           | Fix                                        |
|-----------------------------|---------------------------------|--------------------------------------------|
| Phone can't see device      | Not advertising                 | Check UART log for "Advertising..."        |
| Board always shows DIMMER_01 | Not provisioned                | Send provision command from phone           |
| Provision fails             | NVM write error                 | Check UART for "Stored" confirmation       |
| Local can't reach gateway   | Gateway not powered/in range    | Check gateway UART for incoming connection  |
| Cross-cluster cmd fails     | Gateway backbone not formed     | Verify gateways show central connections    |
| LED wrong color             | Pin mapping                     | Verify PB0=Red, PB3=Green, PB5=Blue       |
| Phone disconnects           | Connection parameter conflict   | Check UART for disconnect reason           |
| Command reaches wrong node  | Node ID in hex, not decimal     | Node 15 = `0F` not `15` in the command     |
| Board stuck after provision | NVM address conflict            | Unprovision with `00 00 00 05 06`, retry   |

## Limitations

- Software PWM (100Hz, 10 brightness levels) — acceptable for dimming evaluation
- Node IDs limited to 1-99 (2-digit display in BLE advertisement name)
- No persistent bonding — connections re-establish on every power cycle
- No encryption on mesh data — suitable for evaluation, add security for production
- Gateway is single point of failure per cluster — if gateway fails, cluster is isolated
- Gateway backbone is sequential (chain) — for better redundancy, add ring topology
- BLE range ~30m indoor — place gateways within range of adjacent gateways
- Provisioning requires phone connection to each board individually
