# BLE Terminal

A small cross-platform GUI to scan for, connect to, and exchange commands with a
BLE peripheral such as the WBZ351. Built on [`bleak`](https://github.com/hbldh/bleak)
(BLE) and tkinter (GUI, bundled with standard Python).

## Install & run

```bash
pip install -r requirements.txt
python ble_terminal.py
```

Requirements: Python 3.9+ and a BLE adapter. On Windows, Bluetooth must be on and
the app may prompt for Bluetooth permission the first time.

## Use

1. **Scan** — lists nearby BLE devices; pick yours from the drop-down.
2. **Connect** — connects and discovers services. The app auto-selects a known
   UART-over-BLE profile if present:
   - Microchip Transparent UART Service (TRS)
   - Nordic UART Service (NUS)

   If your firmware uses something else, choose the **Notify (RX)** and
   **Write (TX)** characteristics manually from the two drop-downs.
3. **Send / receive** — type in the input box and press Enter or **Send**.
   Incoming notifications stream into the terminal.

## Hex / ASCII

Independent format selectors for each direction:

- **Receive: ASCII | Hex** — how incoming bytes are displayed.
- **Send: ASCII | Hex** — how your input is interpreted.
  - *ASCII*: text is UTF-8 encoded; the line-ending drop-down (None/LF/CR/CRLF)
    is appended.
  - *Hex*: input is parsed as byte pairs, e.g. `01 A2 FF` (spaces, commas,
    dashes, and `0x` are ignored). No line ending is added.

Other options: per-line timestamps, autoscroll, and clear.

## Logging & history

- **Save log...** — writes the current terminal contents to a text file.
- **Log to file** — pick a file and every subsequent line (RX/TX/info, with
  timestamps) is appended live until you untick it. Reopening the same file
  appends a new `==== session started ... ====` marker rather than overwriting.
- **Command history** — press **Up / Down** in the input box to recall and
  re-send previous commands.

## Quick commands (one-click macros)

The right-hand **Quick commands** panel holds editable buttons that each send a
full hex frame in one click (raw bytes — the Send ASCII/Hex selector and line
endings do not apply). It comes preloaded with the provisioning and LED-control
commands.

- **Edit...** opens a dialog. One command per line:

  ```
  Label | HEX BYTES
  ```

  A line with no `|` becomes a **section header**. Hex may include spaces, e.g.
  `Assign Node 1 | 00 00 00 05 05 01`. **Save** rebuilds the buttons;
  **Restore defaults** reloads the built-in set.
- Buttons are stored in **`macros.txt`** next to the script, so your edits
  persist across restarts. A button whose hex is invalid is shown disabled and
  labelled `(bad hex)`.

## LED state gauge

The **LED state** panel (top-right) shows the last color you commanded: a color
swatch plus R/G/B level bars with percentages, and the target it was sent to.

It updates whenever a **dimmer frame** is sent (macro button or manual), i.e.
an 8-byte frame of the form `DST 00 00 05 01 R G B`, where `R`/`G`/`B` are
`0x00–0x64` (0–100%). The target byte is decoded as:

- `0x00` -> connected board, `0xFF` -> ALL nodes
- `0xC0–0xCF` -> Cluster 0–15
- anything else -> Node *n* (decimal)

Non-LED frames (identify, provisioning) are ignored by the gauge.

## Color picker

The **Color picker** panel (right side) builds and sends an LED dimmer frame
from R/G/B sliders (0–100%):

- **To:** choose the destination — Connected (`0x00`), ALL (`0xFF`),
  Cluster (0–15 → `0xC0+n`), or Node (1–254). The spin-box sets the number for
  Cluster/Node.
- **Send color** transmits `DST 00 00 05 01 R G B`. Tick **Live** to send
  continuously while you drag (throttled to ~130 ms).

## Network map

Click **Network map** (top bar) to open a live topology view built from the
frames you send. Node IDs imply structure by a fixed rule taken from the
provisioning table:

- `cluster = (id - 1) // 10`
- a node is a **gateway** when `id % 10 == 1` (e.g. 1, 11, 21…)

The map draws a central **MESH** hub, one **gateway** (★) per cluster, and the
**local** nodes around each gateway. Nodes are filled with their last commanded
color; the BLE-connected board gets a yellow ring.

The model updates from sent frames:

- **Provision** (`00 00 00 05 05 <id>`) registers the connected board as node
  *id*; **Unprovision** clears that marker.
- **LED set** colors the addressed node(s) — unicast, cluster (`0xC0–0xCF`), or
  all (`0xFF`).
- **Identify** (`<id> 00 00 05 04`) adds that node.

**Click any node** for actions: Identify, Set color... (opens a color chooser),
Turn off, or Remove from map. You can also **Add node...** manually or **Clear**
the map. Because a single BLE
link only reaches one board, this is a *logical* map from the commands issued —
not a read-back of the live mesh (the firmware would need to report neighbors
for that; tell me the frame format if it does and I'll drive the map from RX).

## Notes

- Writes use an acknowledged write when the characteristic supports `write`,
  otherwise write-without-response — chosen automatically.
- The default WBZ351 Transparent UART UUIDs are already in `KNOWN_PROFILES` in
  `ble_terminal.py`; add your own there to auto-select a custom service.
