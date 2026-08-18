#include <string.h>
#include <stdbool.h>
#include "mesh_routing.h"
#include "mesh_conn_mgr.h"
#include "led_dimmer.h"
#include "node_config.h"
#include "provisioning.h"
#include "osal/osal_freertos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ble_trspc/ble_trspc.h"
#include "ble_trsps/ble_trsps.h"
#include "definitions.h"
#include "app.h"
#include "app_ble_callbacks.h"
#include "app_ble.h"

static uint8_t s_myNodeId;
static uint8_t s_seqNum;
/* Indexed by source node id: per-source state cannot be evicted by another
   node's traffic, which is what made the old shared FIFO collapse. */
static MeshDupEntry_T s_dupCache[NODE_ID_MAX + 1];

static uint16_t s_lastPhoneConnHandle = 0xFFFF;

/* One slot per unicast command awaiting its ACK. Sized for a burst of
   individually addressed commands; a scene over 100 nodes is meant to go out
   as a broadcast or on a cluster address (0xC0..0xC9 covers exactly the ten
   clusters of ten that NODE_ID_MAX allows), neither of which is tracked here. */
#define MESH_TX_QUEUE_SIZE       24U
/* Round-trip budget for the first attempt. The floor on per-hop latency is the
   MESH_Maintenance period, not the connection interval: a packet that misses
   its immediate send waits a whole cycle in s_linkTxQueue. At four levels deep
   an eight-hop round trip is ~2 s even at the 250 ms cycle, so 700 ms expired
   before the ACK could physically arrive and every command re-flooded the
   whole network at least once for nothing. */
#define MESH_ACK_TIMEOUT_TICKS   pdMS_TO_TICKS(2000)
#define MESH_MAX_RETRIES         3U
typedef struct {
    bool inUse;
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t retries;
    uint32_t nextRetryTick;
} MeshPendingTx_T;
static MeshPendingTx_T s_pendingTx[MESH_TX_QUEUE_SIZE];
typedef struct {
    bool inUse;
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t repeats;
    uint32_t nextTick;
} MeshBroadcastRetry_T;
static MeshBroadcastRetry_T s_broadcastRetry;
/* Shared by every link, so one broadcast fan-out that finds the transports
   busy can occupy one slot per link at once. With five links and several
   floods in flight at 100 nodes, 16 overflowed into "TX queue FULL" and
   silently dropped mesh traffic. */
#define MESH_LINK_TX_QUEUE_SIZE 32U
/* A link whose GATT transport never opens (peer has not enabled the CCCD)
   fails every send with MBA_RES_BAD_STATE. Without an age limit its packets
   stay queued for ever and starve every other link out of the shared queue,
   which shows up as "TX queue FULL" and silently dropped mesh traffic. */
/* Has to outlast the retry ladder below it: attempts back off 100 ms << n, so
   six attempts span ~6.3 s and a 5 s age limit was discarding packets that
   still had tries left. */
#define MESH_LINK_TX_MAX_AGE_TICKS pdMS_TO_TICKS(8000)
typedef struct {
    bool inUse;
    uint16_t connHandle;
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t attempts;
    uint32_t nextTick;
    uint32_t enqueueTick;
} MeshLinkTx_T;
static MeshLinkTx_T s_linkTxQueue[MESH_LINK_TX_QUEUE_SIZE];
static uint8_t s_rootId;
static uint8_t s_depth;
static uint8_t s_parentId;
static uint32_t s_nextHeartbeatTick;
static uint32_t s_lastFullReportTick;
static uint32_t s_nextReportTick;
static uint32_t s_prngState;
static bool s_topologyDirty;
static bool s_fullReportPending;
static uint8_t s_reportRetryExp;
/* "Network formed" is a local, conservative judgement: this node has at least
   one mesh link that is fully usable, no mesh link is still mid-JOIN, and it
   has a place in the tree. It has to hold continuously for this long before it
   is announced, so a link that comes up and immediately drops does not produce
   a FORMED/LOST pair on the console. */
#define MESH_FORMED_STABLE_TICKS pdMS_TO_TICKS(5000)
static bool s_networkFormed;
static uint32_t s_formedSinceTick;
#define MESH_HEARTBEAT_BASE_MS  30000UL
#define MESH_HEARTBEAT_JITTER_MS 5000UL
#define MESH_LINK_DEAD_TICKS     pdMS_TO_TICKS(90000)
#define MESH_FULL_REPORT_TICKS   pdMS_TO_TICKS(300000)
#define MESH_REPORT_DEBOUNCE_MS  2000UL
/* Must not be smaller than MESH_MAX_MESH_CHILDREN: mesh_UpdateChildSummary
   silently does nothing when it finds no free slot, so at 3 the fourth child
   of every node was missing from the aggregate that goes up in the topology
   report - the root could never add up to the real network size. */
#define MESH_CHILD_SUMMARY_SIZE  MESH_MAX_MESH_CHILDREN
typedef struct {
    uint8_t nodeId;
    uint8_t nodes;
    uint8_t links;
} MeshChildSummary_T;
static MeshChildSummary_T s_childSummary[MESH_CHILD_SUMMARY_SIZE];

static bool mesh_SendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen);
static bool mesh_TrySendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen);
/* Result of the most recent mesh_TrySendToConn, so the caller can tell
   congestion (worth queuing) from a structural refusal (not worth queuing). */
static uint16_t s_lastSendStatus;

/* Simultaneous startup can make both nodes initiate towards each other before
   either JOIN is visible. Keep exactly one physical link per pair: the higher
   node id owns the central role and the lower id owns the peripheral role. */
static bool mesh_ResolveDuplicatePeerLink(MeshConn_T *current, uint8_t peerId)
{
    uint8_t i;
    ConnRole_T wantedRole = (s_myNodeId > peerId) ?
        CONN_ROLE_CENTRAL : CONN_ROLE_PERIPHERAL;
    MeshConn_T *table = CONN_MGR_GetTable();

    if (current == NULL || peerId == 0U)
        return true;

    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        MeshConn_T *other = &table[i];
        if (!other->inUse || other->connHandle == current->connHandle ||
            other->peerNodeId != peerId || other->type != CONN_TYPE_LOCAL)
            continue;

        if (current->role != wantedRole)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                "Duplicate peer %u: drop hdl=0x%04X role=%c\r\n", peerId,
                current->connHandle,
                (current->role == CONN_ROLE_CENTRAL) ? 'C' : 'P');
            (void)BLE_GAP_Disconnect(current->connHandle, 0x13);
            return false;
        }

        SYS_DEBUG_PRINT(SYS_ERROR_INFO,
            "Duplicate peer %u: drop hdl=0x%04X role=%c\r\n", peerId,
            other->connHandle,
            (other->role == CONN_ROLE_CENTRAL) ? 'C' : 'P');
        (void)BLE_GAP_Disconnect(other->connHandle, 0x13);
    }
    return true;
}

/* Child slots currently spoken for. Two views of the same thing have to be
   combined: children that have joined (typed LOCAL) and children that are
   merely connected but were recognised as mesh nodes at connect time. Counting
   only the joined ones under-reports during formation, which is what let five
   children occupy every peripheral slot and left none for the app. */
static uint8_t mesh_UsedChildSlots(void)
{
    uint8_t connected = CONN_MGR_GetMeshChildCount();
    uint8_t joined = CONN_MGR_GetMeshPeripheralCount();
    return (joined > connected) ? joined : connected;
}

/* A scanning peer connects to us as central, so it consumes one of our
   peripheral slots. Advertise the admission capacity actually enforced in the
   JOIN_REQUEST handler, otherwise peers keep connecting to full nodes. */
static uint8_t mesh_FreeSlots(void)
{
    uint8_t used = mesh_UsedChildSlots();
    return (used < MESH_MAX_MESH_CHILDREN) ?
        (uint8_t)(MESH_MAX_MESH_CHILDREN - used) : 0U;
}

/* Advertised flags: 0x01 mesh node, 0x02 has a place in the tree, 0x04 network
   formed. The last one lets the GUI show mesh health from a plain scan, before
   it connects to anything. */
static void mesh_UpdateAdvertisement(void)
{
    uint8_t flags = 0x01U;
    if (s_parentId != 0U || s_rootId == s_myNodeId) flags |= 0x02U;
    if (s_networkFormed) flags |= 0x04U;
    APP_BLE_UpdateTopologyAdvertisement(s_rootId, s_depth, mesh_FreeSlots(), flags);
}

static uint32_t mesh_Random(void)
{
    s_prngState ^= s_prngState << 13;
    s_prngState ^= s_prngState >> 17;
    s_prngState ^= s_prngState << 5;
    return s_prngState;
}

static uint32_t mesh_HeartbeatDelay(void)
{
    uint32_t span = (MESH_HEARTBEAT_JITTER_MS * 2UL) + 1UL;
    return pdMS_TO_TICKS((MESH_HEARTBEAT_BASE_MS - MESH_HEARTBEAT_JITTER_MS) +
        (mesh_Random() % span));
}

static void mesh_MarkTopologyDirty(void)
{
    s_topologyDirty = true;
    s_nextReportTick = xTaskGetTickCount() + pdMS_TO_TICKS(MESH_REPORT_DEBOUNCE_MS +
        (mesh_Random() % 2001UL));
}

/* queueIfBusy=false sends only if the GATT transport is open right now. Use it
   for traffic that already owns a retry policy, so a failed attempt is not
   silently parked in the shared link queue where it may later expire. */
static bool mesh_SendDirectEx(uint16_t connHandle, uint8_t dstId, uint8_t cmd,
    const uint8_t *payload, uint8_t payloadLen, bool queueIfBusy)
{
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    MeshHeader_T *hdr = (MeshHeader_T *)packet;
    MeshConn_T *conn = CONN_MGR_GetByHandle(connHandle);
    if (conn == NULL || payloadLen > MESH_MAX_PACKET_SIZE - MESH_HEADER_SIZE) return false;
    hdr->dst_id = dstId;
    hdr->src_id = s_myNodeId;
    hdr->seq_num = s_seqNum++;
    hdr->ttl = 1U;
    hdr->cmd = cmd;
    if (payloadLen) memcpy(packet + MESH_HEADER_SIZE, payload, payloadLen);
    if (queueIfBusy)
        return mesh_SendToConn(conn, packet, MESH_HEADER_SIZE + payloadLen);
    return mesh_TrySendToConn(conn, packet, MESH_HEADER_SIZE + payloadLen);
}

static bool mesh_SendDirect(uint16_t connHandle, uint8_t dstId, uint8_t cmd,
    const uint8_t *payload, uint8_t payloadLen)
{
    return mesh_SendDirectEx(connHandle, dstId, cmd, payload, payloadLen, true);
}

static MeshConn_T *mesh_FindParentConnection(void)
{
    uint8_t i;
    MeshConn_T *table = CONN_MGR_GetTable();
    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
        if (table[i].inUse && table[i].isReady && table[i].peerNodeId == s_parentId)
            return &table[i];
    return NULL;
}

static void mesh_UpdateChildSummary(uint8_t childId, uint8_t nodes, uint8_t links)
{
    uint8_t i, empty = 0xFFU;
    for (i = 0U; i < MESH_CHILD_SUMMARY_SIZE; i++)
    {
        if (s_childSummary[i].nodeId == childId)
        {
            s_childSummary[i].nodes = nodes;
            s_childSummary[i].links = links;
            mesh_MarkTopologyDirty();
            return;
        }
        if (s_childSummary[i].nodeId == 0U) empty = i;
    }
    if (empty != 0xFFU)
    {
        s_childSummary[empty].nodeId = childId;
        s_childSummary[empty].nodes = nodes;
        s_childSummary[empty].links = links;
        mesh_MarkTopologyDirty();
    }
}

#define MESH_DUP_MAX_AGE_TICKS  pdMS_TO_TICKS(10000)

static bool mesh_IsClusterAddress(uint8_t dstId)
{
    return (dstId >= MESH_CLUSTER_ADDR_BASE) &&
           (dstId <= MESH_CLUSTER_ADDR_MAX);
}

static bool mesh_IsMyCluster(uint8_t dstId)
{
    return mesh_IsClusterAddress(dstId) &&
           ((dstId - MESH_CLUSTER_ADDR_BASE) == GET_CLUSTER(s_myNodeId));
}

/* Sequence numbers are 8 bit and wrap, so ordering is a signed difference.
   Returns NULL for a source id that cannot exist. */
static MeshDupEntry_T *mesh_DupEntry(uint8_t srcId)
{
    if (srcId < NODE_ID_MIN || srcId > NODE_ID_MAX) return NULL;
    return &s_dupCache[srcId];
}

static bool mesh_IsDuplicate(uint8_t srcId, uint8_t seqNum)
{
    MeshDupEntry_T *e = mesh_DupEntry(srcId);
    int8_t diff;

    if (e == NULL || !e->valid) return false;
    /* A source that has been silent long enough may have rebooted and
       restarted its counter, so its history is no longer meaningful. */
    if ((xTaskGetTickCount() - e->tickStamp) >= MESH_DUP_MAX_AGE_TICKS)
        return false;

    diff = (int8_t)(seqNum - e->lastSeq);
    if (diff > 0) return false;                          /* newer than anything seen */
    if (diff <= -(int8_t)MESH_DUP_WINDOW) return false;  /* older than the window */
    return (e->window & (1ULL << (uint8_t)(-diff))) != 0ULL;
}

static void mesh_AddToDupCache(uint8_t srcId, uint8_t seqNum)
{
    MeshDupEntry_T *e = mesh_DupEntry(srcId);
    uint32_t now = xTaskGetTickCount();
    int8_t diff;

    if (e == NULL) return;

    if (!e->valid || (now - e->tickStamp) >= MESH_DUP_MAX_AGE_TICKS)
    {
        e->valid = true;
        e->lastSeq = seqNum;
        e->window = 1ULL;      /* bit 0 tracks lastSeq itself */
        e->tickStamp = now;
        return;
    }

    diff = (int8_t)(seqNum - e->lastSeq);
    if (diff > 0)
    {
        e->window = (diff >= (int8_t)MESH_DUP_WINDOW) ?
            1ULL : ((e->window << (uint8_t)diff) | 1ULL);
        e->lastSeq = seqNum;
    }
    else if (diff > -(int8_t)MESH_DUP_WINDOW)
    {
        e->window |= (1ULL << (uint8_t)(-diff));
    }
    else
    {
        /* Further than the window in either direction. An 8 bit counter cannot
           tell a big jump forward from an equally big step back, and either
           way the stored history no longer orders against this packet, so
           resync onto it. Without this the entry stays pinned to a stale
           lastSeq and stops detecting duplicates altogether. */
        e->window = 1ULL;
        e->lastSeq = seqNum;
    }
    e->tickStamp = now;
}

static bool mesh_SendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen)
{
    uint8_t i;
    if (mesh_TrySendToConn(conn, p_packet, packetLen))
        return true;

    /* Only congestion is worth queuing. A structural refusal - the transport
       is not open yet, or GATT rejected the attribute handle - will refuse the
       retry identically, so queuing it just occupies a shared slot until it
       expires. s_lastSendStatus is set by mesh_TrySendToConn. */
    if (s_lastSendStatus != MBA_RES_NO_RESOURCE &&
        s_lastSendStatus != MBA_RES_BAD_STATE)
        return false;

    if (packetLen > MESH_MAX_PACKET_SIZE)
        return false;
    for (i = 0U; i < MESH_LINK_TX_QUEUE_SIZE; i++)
    {
        if (!s_linkTxQueue[i].inUse)
        {
            s_linkTxQueue[i].inUse = true;
            s_linkTxQueue[i].connHandle = conn->connHandle;
            memcpy(s_linkTxQueue[i].packet, p_packet, packetLen);
            s_linkTxQueue[i].length = (uint8_t)packetLen;
            s_linkTxQueue[i].attempts = 0U;
            s_linkTxQueue[i].enqueueTick = xTaskGetTickCount();
            s_linkTxQueue[i].nextTick = xTaskGetTickCount() + pdMS_TO_TICKS(100);
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TX queued hdl=0x%04X cmd=0x%02X\r\n",
                conn->connHandle, p_packet[4]);
            return true;
        }
    }
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TX queue FULL hdl=0x%04X\r\n", conn->connHandle);
    return false;
}

static bool mesh_TrySendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen)
{
    uint16_t status = 0xFFFF;
    if (conn->role == CONN_ROLE_CENTRAL)
        status = BLE_TRSPC_SendData(conn->connHandle, packetLen, p_packet);
    else if (conn->role == CONN_ROLE_PERIPHERAL)
        status = BLE_TRSPS_SendData(conn->connHandle, packetLen, p_packet);

    s_lastSendStatus = status;
    if (status != 0)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Send FAIL hdl=0x%04X err=0x%04X\r\n",
            conn->connHandle, status);
        return false;
    }
    return true;
}

static void mesh_ForwardToLocals(uint8_t *p_packet, uint16_t packetLen, uint16_t excludeConnHandle)
{
    uint8_t i;
    MeshConn_T *table = CONN_MGR_GetTable();
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse || !table[i].isReady || !table[i].topologyReady) continue;
        if (table[i].connHandle == excludeConnHandle) continue;
        if (table[i].type == CONN_TYPE_LOCAL)
            mesh_SendToConn(&table[i], p_packet, packetLen);
    }
}

static void mesh_ForwardToGateways(uint8_t *p_packet, uint16_t packetLen, uint16_t excludeConnHandle)
{
    uint8_t i;
    MeshConn_T *table = CONN_MGR_GetTable();
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse || !table[i].isReady || !table[i].topologyReady) continue;
        if (table[i].connHandle == excludeConnHandle) continue;
        if (table[i].type == CONN_TYPE_GATEWAY)
            mesh_SendToConn(&table[i], p_packet, packetLen);
    }
}

static void mesh_ForwardToGateway(uint8_t *p_packet, uint16_t packetLen, uint16_t excludeConnHandle)
{
    uint8_t i;
    MeshConn_T *table = CONN_MGR_GetTable();
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse || !table[i].isReady || !table[i].topologyReady) continue;
        if (table[i].connHandle == excludeConnHandle) continue;
        if (table[i].type == CONN_TYPE_GATEWAY)
        {
            mesh_SendToConn(&table[i], p_packet, packetLen);
            return;
        }
    }
}

static void mesh_ForwardAll(uint8_t *p_packet, uint16_t packetLen, uint16_t excludeConnHandle)
{
    uint8_t i;
    uint8_t sent = 0;
    MeshConn_T *table = CONN_MGR_GetTable();
    MeshHeader_T *forwardHdr = (MeshHeader_T *)p_packet;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse) continue;
        if (!table[i].isReady)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "FWD skip hdl=0x%04X not ready\r\n", table[i].connHandle);
            continue;
        }
        if (table[i].type == CONN_TYPE_LOCAL && !table[i].topologyReady &&
            forwardHdr->dst_id != MESH_BROADCAST_ADDR &&
            !mesh_IsClusterAddress(forwardHdr->dst_id))
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "FWD skip hdl=0x%04X join pending\r\n",
                table[i].connHandle);
            continue;
        }
        if (table[i].connHandle == excludeConnHandle) continue;
        if (table[i].type == CONN_TYPE_PHONE) continue;
        if (mesh_SendToConn(&table[i], p_packet, packetLen))
            sent++;
    }
    if (sent == 0)
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "FWD: no peers to forward to\r\n");
    else
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "FWD: sent to %d peers\r\n", (int)sent);
}

static bool mesh_CommandNeedsAck(uint8_t cmd)
{
    return cmd == MESH_CMD_SET_DIMMER || cmd == MESH_CMD_GET_STATUS ||
           cmd == MESH_CMD_IDENTIFY || cmd == MESH_CMD_PROVISION ||
           cmd == MESH_CMD_UNPROVISION;
}

static void mesh_SendAck(const MeshHeader_T *received)
{
    uint8_t payload[2] = { received->src_id, received->seq_num };
    MESH_SendCommand(received->src_id, MESH_CMD_ACK, payload, sizeof(payload));
}

static void mesh_ConfirmAck(const uint8_t *payload, uint8_t payloadLen)
{
    uint8_t i;
    if (payloadLen < 2U || payload[0] != s_myNodeId) return;
    for (i = 0; i < MESH_TX_QUEUE_SIZE; i++)
    {
        MeshHeader_T *hdr = (MeshHeader_T *)s_pendingTx[i].packet;
        if (s_pendingTx[i].inUse && hdr->seq_num == payload[1])
        {
            s_pendingTx[i].inUse = false;
            return;
        }
    }
}

/* A JOIN or HELLO is the peer stating it is a mesh node, and it is the only
   evidence that survives a staggered power-up: a node whose mesh has already
   formed has stopped scanning, so a newcomer's address is not in its discovery
   table and the connect-time guess classified the link as the phone. Left
   uncorrected, the link kept the app connection parameters - peripheral
   latency 4 and a 5 s supervision timeout - and dropped with reason 0x08 until
   the newcomer had backed itself off out of the network. */
static void mesh_ConfirmMeshLink(uint16_t connHandle)
{
    MeshConn_T *conn = CONN_MGR_GetByHandle(connHandle);
    if (conn == NULL || conn->meshPeer)
        return;
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Late peer on hdl=0x%04X, reclassifying\r\n",
        connHandle);
    CONN_MGR_SetMeshPeer(connHandle, true);
    APP_BLE_ApplyLinkConnParams(connHandle);
}

/* A joiner carrying a lower node id than ours is not a child - it is a better
   parent that happens to have reached us first. Adopt it over the link that
   already exists.
 *
 * This is what removes the commissioning-order constraint. Edges are created
 * by whoever is scanning, so a node powered up after its neighbours can only
 * reach them upwards, and the old rule (adopt a parent only on a link we
 * opened as central) left it permanently rootless: the mesh ended up with one
 * root per commissioning step. Nothing in the routing requires the parent to
 * be the central side - mesh_TrySendToConn picks TRSPC or TRSPS from the role,
 * and mesh_FindParentConnection matches on peer id - so the tree can reorder
 * logically, with no disconnect and no role swap.
 *
 * Cycle safety is unchanged: the parent is still always a strictly lower node
 * id, so the parent chain cannot close on itself. The payload is the JOIN
 * body: joiner id, its root, its depth. */
static bool mesh_AdoptJoinerAsParent(const uint8_t *payload)
{
    if (payload[0] >= s_myNodeId)
        return false;
    if (payload[1] >= s_rootId && s_parentId != 0U)
        return false;

    s_parentId = payload[0];
    s_rootId = payload[1];
    s_depth = (payload[2] < 0xFEU) ? (uint8_t)(payload[2] + 1U) : 0xFFU;
    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
        "Adopted DIMMER_%02d as parent (uplink) root=%u depth=%u\r\n",
        s_parentId, s_rootId, s_depth);
    mesh_UpdateAdvertisement();
    mesh_MarkTopologyDirty();
    return true;
}

static void mesh_ExecuteCommand(MeshHeader_T *pHdr, uint8_t *payload, uint8_t payloadLen, uint16_t connHandle)
{
    switch (pHdr->cmd)
    {
        case MESH_CMD_HELLO:
            if (payloadLen >= 1U)
            {
                MeshConn_T *helloConn = CONN_MGR_GetByHandle(connHandle);
                CONN_MGR_SetPeerNodeId(connHandle, payload[0]);
                CONN_MGR_SetType(connHandle, CONN_TYPE_LOCAL);
                mesh_ConfirmMeshLink(connHandle);
                CONN_MGR_SetReady(connHandle);
                if (!mesh_ResolveDuplicatePeerLink(helloConn, payload[0]))
                    break;
                CONN_MGR_SetTopologyReady(connHandle);
            }
            break;

        case MESH_CMD_JOIN_REQUEST:
            if (payloadLen >= 3U)
            {
                uint8_t reply[3];
                MeshConn_T *joiner = CONN_MGR_GetByHandle(connHandle);
                bool alreadyAdmitted = (joiner != NULL) && joiner->topologyReady;
                CONN_MGR_SetPeerNodeId(connHandle, payload[0]);
                CONN_MGR_SetType(connHandle, CONN_TYPE_LOCAL);
                mesh_ConfirmMeshLink(connHandle);
                CONN_MGR_SetReady(connHandle);
                if (!mesh_ResolveDuplicatePeerLink(joiner, payload[0]))
                    break;
                /* Decided before the quota check on purpose: an uplink is not
                   a child, so a full node must still accept a lower id that
                   improves its place in the tree. Refusing it would recreate
                   the very partition this is here to prevent. */
                CONN_MGR_SetPeerTopology(connHandle, payload[1], payload[2],
                    0xFFU);
                bool asUplink = mesh_AdoptJoinerAsParent(payload);
                /* Admission is decided once. A repeated JOIN on a child that
                   is already in the tree (its JOIN_ACCEPT was lost, or its
                   stack re-raised the event) must not be re-evaluated: by then
                   this node may be full, and the answer would be a redirect
                   that disconnects a healthy, established link.
                   SetType(LOCAL) above already counts this connection. */
                if (!alreadyAdmitted && !asUplink &&
                    mesh_UsedChildSlots() > MESH_MAX_MESH_CHILDREN)
                {
                    uint8_t redirect[2] = { s_rootId, s_parentId };
                    mesh_SendDirect(connHandle, payload[0], MESH_CMD_JOIN_REDIRECT,
                        redirect, sizeof(redirect));
                    (void)BLE_GAP_Disconnect(connHandle, 0x13);
                    break;
                }
                CONN_MGR_SetTopologyReady(connHandle);
                reply[0] = s_rootId;
                reply[1] = s_depth;
                reply[2] = mesh_FreeSlots();
                mesh_SendDirect(connHandle, payload[0], MESH_CMD_JOIN_ACCEPT,
                    reply, sizeof(reply));
                mesh_UpdateAdvertisement();
                mesh_MarkTopologyDirty();
            }
            break;

        case MESH_CMD_JOIN_REDIRECT:
            APP_BLE_MarkPeerRejected(pHdr->src_id);
            (void)BLE_GAP_Disconnect(connHandle, 0x13);
            break;

        case MESH_CMD_JOIN_ACCEPT:
            if (payloadLen >= 2U)
            {
                MeshConn_T *joinConn = CONN_MGR_GetByHandle(connHandle);
                if (joinConn != NULL)
                {
                    CONN_MGR_SetPeerNodeId(connHandle, pHdr->src_id);
                    CONN_MGR_SetType(connHandle, CONN_TYPE_LOCAL);
                    CONN_MGR_SetReady(connHandle);
                    CONN_MGR_SetTopologyReady(connHandle);
                    CONN_MGR_SetPeerTopology(connHandle, payload[0], payload[1],
                        (payloadLen >= 3U) ? payload[2] : 0xFFU);
                }
                /* Adopt as parent only towards a lower node id, the same rule
                   MESH_SendHello applies. The parent relation must be strictly
                   decreasing or it can close a cycle - which it now can, since
                   an isolated node is allowed to link upwards. A cycle makes
                   depth chase itself upwards for ever (count to infinity) and
                   every step floods a topology report. The link itself stays
                   fully usable for forwarding; it just is not the parent. */
                if ((pHdr->src_id < s_myNodeId) &&
                    (payload[0] < s_rootId || s_parentId == 0U))
                {
                    s_rootId = payload[0];
                    s_depth = (payload[1] < 0xFEU) ? (uint8_t)(payload[1] + 1U) : 0xFFU;
                    s_parentId = pHdr->src_id;
                    mesh_UpdateAdvertisement();
                    mesh_MarkTopologyDirty();
                }
            }
            break;

        case MESH_CMD_HEARTBEAT:
            CONN_MGR_Touch(connHandle);
            if (payloadLen >= 3U)
                CONN_MGR_SetPeerTopology(connHandle, payload[0], payload[1],
                    payload[2]);
            /* Echo the per-link token on the same physical connection. This
               proves both application directions are alive; a BLE connection
               event alone cannot detect a stalled TRSP/CBFC data path. */
            if (payloadLen >= 4U && connHandle != 0xFFFFU)
                (void)mesh_SendDirect(connHandle, pHdr->src_id,
                    MESH_CMD_HEARTBEAT_ACK, &payload[3], 1U);
            if (payloadLen >= 2U && pHdr->src_id == s_parentId &&
                (payload[0] != s_rootId || (uint8_t)(payload[1] + 1U) != s_depth))
            {
                s_rootId = payload[0];
                s_depth = (uint8_t)(payload[1] + 1U);
                mesh_UpdateAdvertisement();
                mesh_MarkTopologyDirty();
            }
            break;

        case MESH_CMD_HEARTBEAT_ACK:
            if (payloadLen >= 1U)
            {
                MeshConn_T *ackConn = CONN_MGR_GetByHandle(connHandle);
                if (ackConn != NULL && ackConn->heartbeatAwaitingAck &&
                    ackConn->heartbeatToken == payload[0])
                {
                    ackConn->heartbeatAwaitingAck = false;
                    ackConn->heartbeatMisses = 0U;
                    CONN_MGR_Touch(connHandle);
                }
            }
            break;

        case MESH_CMD_TOPOLOGY_REPORT:
            if (payloadLen >= 7U)
            {
                CONN_MGR_Touch(connHandle);
                mesh_UpdateChildSummary(pHdr->src_id, payload[4], payload[5]);
            }
            break;
        case MESH_CMD_REPAIR_REQUEST:
            CONN_MGR_Touch(connHandle);
            break;
        case MESH_CMD_ACK:
            mesh_ConfirmAck(payload, payloadLen);
            break;
        case MESH_CMD_SET_DIMMER:
        {
            if (payloadLen >= 3)
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "SET R=%d G=%d B=%d\r\n",
                    (int)payload[0], (int)payload[1], (int)payload[2]);
                LED_Dimmer_SetRGB(payload[0], payload[1], payload[2]);
            }
        }
        break;

        case MESH_CMD_GET_STATUS:
        {
            uint8_t respPayload[4];
            LED_Dimmer_GetRGB(&respPayload[0], &respPayload[1], &respPayload[2]);
            respPayload[3] = CONN_MGR_GetActiveCount();
            if (connHandle != 0xFFFFU && s_lastPhoneConnHandle == connHandle)
            {
                uint8_t response[MESH_HEADER_SIZE + 4U];
                MeshHeader_T *responseHdr = (MeshHeader_T *)response;
                responseHdr->dst_id = NODE_ID_PHONE;
                responseHdr->src_id = s_myNodeId;
                responseHdr->seq_num = s_seqNum++;
                responseHdr->ttl = MESH_MAX_TTL;
                responseHdr->cmd = MESH_CMD_STATUS_RESP;
                memcpy(&response[MESH_HEADER_SIZE], respPayload, sizeof(respPayload));
                BLE_TRSPS_SendData(connHandle, sizeof(response), response);
            }
            else
            {
                MESH_SendCommand(pHdr->src_id, MESH_CMD_STATUS_RESP, respPayload, 4);
            }
        }
        break;

        case MESH_CMD_STATUS_RESP:
        {
            if (s_lastPhoneConnHandle != 0xFFFF)
            {
                uint8_t fwdBuf[MESH_HEADER_SIZE + 4];
                memcpy(fwdBuf, pHdr, MESH_HEADER_SIZE);
                if (payloadLen <= 4)
                    memcpy(&fwdBuf[MESH_HEADER_SIZE], payload, payloadLen);
                BLE_TRSPS_SendData(s_lastPhoneConnHandle, MESH_HEADER_SIZE + payloadLen, fwdBuf);
            }
        }
        break;

        case MESH_CMD_IDENTIFY:
        {
            LED_Dimmer_Identify();
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "IDENTIFY\r\n");
        }
        break;

        case MESH_CMD_PROVISION:
        {
            if (payloadLen >= 1)
            {
                uint8_t newId = payload[0];
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "PROVISION id=%d\r\n", (int)newId);
                if (!APP_BLE_IsNodeIdObserved(newId) && PROV_SetNodeId(newId))
                {
                    static uint8_t provOk[] = "PROV_OK";
                    LED_Dimmer_SetRGB(0, 100, 0);
                    if (connHandle != 0xFFFFU)
                        (void)BLE_TRSPS_SendData(connHandle, sizeof(provOk) - 1U, provOk);
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Stored. Rebooting...\r\n");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    NVIC_SystemReset();
                }
                else
                {
                    static uint8_t provFail[] = "PROV_FAIL";
                    LED_Dimmer_SetRGB(100, 0, 0);
                    if (connHandle != 0xFFFFU)
                        (void)BLE_TRSPS_SendData(connHandle, sizeof(provFail) - 1U, provFail);
                }
            }
        }
        break;

        case MESH_CMD_UNPROVISION:
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "UNPROVISION\r\n");
            PROV_Reset();
            LED_Dimmer_SetRGB(100, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            NVIC_SystemReset();
        }
        break;

        default:
            break;
    }
}

void MESH_Init(uint8_t myNodeId)
{
    s_myNodeId = myNodeId;
    s_seqNum = 0;
    memset(s_dupCache, 0, sizeof(s_dupCache));
    memset(s_pendingTx, 0, sizeof(s_pendingTx));
    memset(&s_broadcastRetry, 0, sizeof(s_broadcastRetry));
    memset(s_linkTxQueue, 0, sizeof(s_linkTxQueue));
    s_rootId = myNodeId;
    s_depth = 0U;
    s_parentId = 0U;
    s_prngState = ((uint32_t)myNodeId << 24) ^ xTaskGetTickCount() ^ 0xA53C9E17UL;
    if (s_prngState == 0U) s_prngState = 1U;
    s_nextHeartbeatTick = xTaskGetTickCount() + mesh_HeartbeatDelay();
    s_lastFullReportTick = xTaskGetTickCount();
    s_nextReportTick = s_lastFullReportTick + pdMS_TO_TICKS(MESH_REPORT_DEBOUNCE_MS);
    s_topologyDirty = true;
    s_fullReportPending = true;
    s_reportRetryExp = 0U;
    s_networkFormed = false;
    s_formedSinceTick = 0U;
    s_lastPhoneConnHandle = 0xFFFFU;
    memset(s_childSummary, 0, sizeof(s_childSummary));
    mesh_UpdateAdvertisement();
}

void MESH_ProcessIncoming(uint16_t connHandle, uint16_t dataLen, uint8_t *p_data)
{
    MeshHeader_T *pHdr;
    uint8_t *payload;
    uint8_t payloadLen;
    bool fromPhone;

    if (dataLen < MESH_HEADER_SIZE)
        return;

    /* Any valid application packet proves that the BLE peer is alive. Do not
       tear down an active RGB/broadcast link merely because a heartbeat was
       delayed or its notification buffer was temporarily unavailable. */
    if (connHandle != 0xFFFFU)
        CONN_MGR_Touch(connHandle);

    pHdr = (MeshHeader_T *)p_data;
    payload = p_data + MESH_HEADER_SIZE;
    payloadLen = (uint8_t)(dataLen - MESH_HEADER_SIZE);

    if (pHdr->ttl == 0)
        return;

    fromPhone = (pHdr->src_id == NODE_ID_PHONE);
    if (fromPhone)
    {
        s_lastPhoneConnHandle = connHandle;
        pHdr->src_id = s_myNodeId;
        pHdr->seq_num = s_seqNum++;
        pHdr->ttl = MESH_MAX_TTL;
        mesh_AddToDupCache(s_myNodeId, pHdr->seq_num);
        /* GATT/CBFC already provides reliable delivery on each physical link.
           Re-injecting every RGB broadcast with a new sequence number made all
           nodes execute and flood it again, tripling traffic during slider
           bursts and starving connection discovery. */
    }
    else
    {
        if (mesh_IsDuplicate(pHdr->src_id, pHdr->seq_num))
        {
            if (pHdr->dst_id == s_myNodeId && mesh_CommandNeedsAck(pHdr->cmd))
                mesh_SendAck(pHdr);
            return;
        }
        mesh_AddToDupCache(pHdr->src_id, pHdr->seq_num);
    }

    uint8_t dstId = pHdr->dst_id;

    if (dstId == s_myNodeId || dstId == MESH_BROADCAST_ADDR ||
        dstId == NODE_ID_PHONE || mesh_IsMyCluster(dstId))
    {
        mesh_ExecuteCommand(pHdr, payload, payloadLen, connHandle);
        if (dstId == s_myNodeId && mesh_CommandNeedsAck(pHdr->cmd))
            mesh_SendAck(pHdr);
    }

    if (dstId == s_myNodeId || dstId == NODE_ID_PHONE)
        return;

    pHdr->ttl--;
    if (pHdr->ttl == 0)
        return;

    mesh_ForwardAll(p_data, dataLen, connHandle);
}

static void mesh_SendCommandTtl(uint8_t dstId, uint8_t cmd, uint8_t *payload,
    uint8_t payloadLen, uint8_t ttl)
{
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    MeshHeader_T *pHdr = (MeshHeader_T *)packet;
    uint16_t totalLen;

    if (payloadLen > (MESH_MAX_PACKET_SIZE - MESH_HEADER_SIZE))
        payloadLen = MESH_MAX_PACKET_SIZE - MESH_HEADER_SIZE;

    pHdr->dst_id = dstId;
    pHdr->src_id = s_myNodeId;
    pHdr->seq_num = s_seqNum++;
    pHdr->ttl = ttl;
    pHdr->cmd = cmd;

    if (payload && payloadLen > 0)
        memcpy(&packet[MESH_HEADER_SIZE], payload, payloadLen);

    totalLen = MESH_HEADER_SIZE + payloadLen;

    mesh_AddToDupCache(s_myNodeId, pHdr->seq_num);
    mesh_ForwardAll(packet, totalLen, 0xFFFF);

    /* Reliable delivery is used for individual commands. Broadcast/cluster
       delivery remains best effort because the sender has no complete member list. */
    if (dstId != MESH_BROADCAST_ADDR && !mesh_IsClusterAddress(dstId) &&
        dstId != NODE_ID_PHONE && cmd != MESH_CMD_ACK && cmd != MESH_CMD_STATUS_RESP)
    {
        uint8_t i, slot = 0xFFU;
        for (i = 0; i < MESH_TX_QUEUE_SIZE; i++)
        {
            if (!s_pendingTx[i].inUse)
            {
                slot = i;
                break;
            }
        }
        /* A new RGB command may evict queued non-RGB traffic, never another
           RGB command. This keeps light control responsive under congestion. */
        if (slot == 0xFFU && cmd == MESH_CMD_SET_DIMMER)
            for (i = 0U; i < MESH_TX_QUEUE_SIZE; i++)
                if (((MeshHeader_T *)s_pendingTx[i].packet)->cmd != MESH_CMD_SET_DIMMER)
                {
                    slot = i;
                    break;
                }
        if (slot != 0xFFU)
        {
            s_pendingTx[slot].inUse = true;
            memcpy(s_pendingTx[slot].packet, packet, totalLen);
            s_pendingTx[slot].length = (uint8_t)totalLen;
            s_pendingTx[slot].retries = 0U;
            s_pendingTx[slot].nextRetryTick = xTaskGetTickCount() + MESH_ACK_TIMEOUT_TICKS;
        }
    }
}

void MESH_SendCommand(uint8_t dstId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen)
{
    mesh_SendCommandTtl(dstId, cmd, payload, payloadLen, MESH_MAX_TTL);
}

/* ------------------------------------------------------------------ *
 * Tree rebalancing
 *
 * Edges are created by whoever happens to be scanning, so the shape of the
 * tree records the commissioning order rather than the radio topology. The
 * pathological case is strictly descending commissioning: every node powers up
 * as the lowest id so far, attaches to the current root, and immediately
 * becomes the new root - producing a chain of depth N-1 instead of a four-ary
 * tree of depth 4.
 *
 * Three mechanisms, in increasing order of cost:
 *
 *   1. Re-parent over a link that already exists. Free: no scan, no new
 *      connection, no teardown. Uses the root/depth every heartbeat already
 *      carries.
 *   2. Scan and connect to a better parent, when the single uplink slot is
 *      free. Costs one scan window.
 *   3. Release an uplink slot held by a link that is no longer a tree edge
 *      below us, so mechanism 2 can run. This is what lets a chain unwind.
 *
 * What makes all of this safe is the invariant that a parent is always a
 * strictly lower node id: the parent graph cannot close a loop no matter what
 * the rebalancer decides, so the worst outcome is a suboptimal depth, never a
 * partitioned or looping network. MESH_REBALANCE_MIN_GAIN is the hysteresis
 * that keeps two comparable parents from trading a node back and forth.
 * ------------------------------------------------------------------ */
#define MESH_REBALANCE_ENABLE       1
/* Below this depth the tree is already good enough to leave alone. */
#define MESH_REBALANCE_MIN_DEPTH    3U
/* Levels a candidate must beat the current parent by before we move. */
#define MESH_REBALANCE_MIN_GAIN     2U
#define MESH_REBALANCE_INFO_MAX_AGE pdMS_TO_TICKS(120000)

/* True when taking a parent at (root, depth) would put this node meaningfully
   higher in the tree than it sits now. Shared with the connection logic so a
   rebalance scan does not spend the uplink slot on a peer that gains nothing. */
bool MESH_WouldImproveDepth(uint8_t peerRootId, uint8_t peerDepth)
{
    if (peerDepth == 0xFFU)
        return false;
    if (peerRootId < s_rootId)
        return true;                       /* a better root always wins */
    if (peerRootId > s_rootId)
        return false;
    if (s_parentId == 0U)
        return true;                       /* no parent: anything is better */
    return ((uint16_t)peerDepth + 1U + MESH_REBALANCE_MIN_GAIN) <=
           (uint16_t)s_depth;
}

bool MESH_WantsRebalance(void)
{
#if MESH_REBALANCE_ENABLE
    return (s_depth >= MESH_REBALANCE_MIN_DEPTH) && (s_depth != 0xFFU);
#else
    return false;
#endif
}

#if MESH_REBALANCE_ENABLE
/* Mechanism 1. Walks the links this node already holds and moves the parent
   pointer to the best of them. Nothing is connected or disconnected. */
static void mesh_RebalanceOverExistingLinks(uint32_t now)
{
    MeshConn_T *table = CONN_MGR_GetTable();
    MeshConn_T *best = NULL;
    uint8_t i;

    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        MeshConn_T *c = &table[i];
        if (!c->inUse || c->type != CONN_TYPE_LOCAL || !c->isReady ||
            !c->topologyReady || c->peerNodeId == 0U)
            continue;
        /* The invariant that keeps the parent graph acyclic. */
        if (c->peerNodeId >= s_myNodeId)
            continue;
        if (c->peerInfoTick == 0U ||
            (now - c->peerInfoTick) >= MESH_REBALANCE_INFO_MAX_AGE)
            continue;                       /* stale or never reported */
        if (c->peerDepth == 0xFFU)
            continue;
        if (best == NULL || c->peerRootId < best->peerRootId ||
            (c->peerRootId == best->peerRootId && c->peerDepth < best->peerDepth))
            best = c;
    }

    if (best == NULL || best->peerNodeId == s_parentId)
        return;
    if (!MESH_WouldImproveDepth(best->peerRootId, best->peerDepth))
        return;

    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
        "Rebalance: parent %u -> %u, depth %u -> %u\r\n",
        s_parentId, best->peerNodeId, s_depth, best->peerDepth + 1U);
    s_parentId = best->peerNodeId;
    s_rootId = best->peerRootId;
    s_depth = (best->peerDepth < 0xFEU) ?
        (uint8_t)(best->peerDepth + 1U) : 0xFFU;
    mesh_UpdateAdvertisement();
    mesh_MarkTopologyDirty();
}

/* Mechanism 3. The single uplink slot is spent on a link this node opened as
   central. If the peer on it reports a depth no deeper than our own, it is not
   below us in the tree any more - it found a shorter path - so the link is not
   carrying our subtree and the slot is better spent looking for a parent.
   Only ever released while another usable mesh link remains, so the node
   cannot isolate itself doing this. */
static void mesh_ReleaseRedundantUplink(uint32_t now)
{
    MeshConn_T *table = CONN_MGR_GetTable();
    uint8_t i;

    if (!MESH_WantsRebalance() || CONN_MGR_GetReadyLocalLinkCount() < 2U)
        return;

    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        MeshConn_T *c = &table[i];
        if (!c->inUse || c->role != CONN_ROLE_CENTRAL ||
            c->type != CONN_TYPE_LOCAL || c->peerNodeId == 0U)
            continue;
        if (c->peerNodeId == s_parentId)
            continue;                       /* this one is our uplink */
        if (c->peerInfoTick == 0U ||
            (now - c->peerInfoTick) >= MESH_REBALANCE_INFO_MAX_AGE)
            continue;
        if (c->peerDepth == 0xFFU || c->peerDepth > s_depth)
            continue;                       /* still below us: keep it */

        SYS_DEBUG_PRINT(SYS_ERROR_INFO,
            "Rebalance: releasing uplink slot held by DIMMER_%02d "
            "(its depth %u <= ours %u)\r\n",
            c->peerNodeId, c->peerDepth, s_depth);
        (void)BLE_GAP_Disconnect(c->connHandle, 0x13);
        return;
    }
}
#endif /* MESH_REBALANCE_ENABLE */

/* This node plus everything reported from below it. Only the root sees the
   whole network; every other node sees its own subtree, which is still the
   useful number to print next to its own FORMED line. */
static uint8_t mesh_SubtreeNodeCount(void)
{
    uint8_t i;
    uint16_t nodes = 1U;
    for (i = 0U; i < MESH_CHILD_SUMMARY_SIZE; i++)
    {
        if (s_childSummary[i].nodeId == 0U) continue;
        nodes += s_childSummary[i].nodes;
    }
    return (nodes > 255U) ? 255U : (uint8_t)nodes;
}

/* Every mesh link is up and admitted, there is at least one of them, and this
   node knows where it sits in the tree. A link that is connected but still
   negotiating fails this deliberately: it is exactly the state where routing
   silently drops packets, so it must not be reported as a formed network. */
static bool mesh_LinksSettled(void)
{
    uint8_t i;
    MeshConn_T *table = CONN_MGR_GetTable();

    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse || table[i].type == CONN_TYPE_PHONE)
            continue;
        /* Either view is enough to make this a mesh link: recognised as one at
           connect time, or typed LOCAL later by its own JOIN. An idle app link
           is neither, and must not hold the whole node in "forming". */
        if (!table[i].meshPeer && table[i].type != CONN_TYPE_LOCAL)
            continue;
        if (!table[i].isReady || !table[i].topologyReady)
            return false;
    }
    return (CONN_MGR_GetReadyLocalLinkCount() > 0U) &&
           (s_parentId != 0U || s_rootId == s_myNodeId);
}

static void mesh_UpdateNetworkFormed(uint32_t now)
{
    if (!mesh_LinksSettled())
    {
        s_formedSinceTick = 0U;
        if (s_networkFormed)
        {
            s_networkFormed = false;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                "*** MESH NOT FORMED *** id=%u links=%u parent=%u root=%u\r\n",
                s_myNodeId, CONN_MGR_GetReadyLocalLinkCount(), s_parentId,
                s_rootId);
            mesh_UpdateAdvertisement();
        }
        return;
    }

    if (s_formedSinceTick == 0U)
    {
        s_formedSinceTick = now;
    }
    else if (!s_networkFormed &&
             (now - s_formedSinceTick) >= MESH_FORMED_STABLE_TICKS)
    {
        s_networkFormed = true;
        SYS_DEBUG_PRINT(SYS_ERROR_INFO,
            "*** MESH FORMED *** id=%u root=%u depth=%u parent=%u links=%u nodes=%u\r\n",
            s_myNodeId, s_rootId, s_depth, s_parentId,
            CONN_MGR_GetReadyLocalLinkCount(), mesh_SubtreeNodeCount());
        mesh_UpdateAdvertisement();
    }
}

bool MESH_IsNetworkFormed(void)
{
    return s_networkFormed;
}

uint8_t MESH_GetSubtreeNodeCount(void)
{
    return mesh_SubtreeNodeCount();
}

/* Called when a link is torn down, before it leaves the connection table, so
   no routing state keeps pointing at a handle the controller is free to hand
   out again to a completely different peer. */
void MESH_OnConnectionClosed(uint16_t connHandle)
{
    uint8_t i;

    if (s_lastPhoneConnHandle == connHandle)
        s_lastPhoneConnHandle = 0xFFFFU;

    for (i = 0U; i < MESH_LINK_TX_QUEUE_SIZE; i++)
        if (s_linkTxQueue[i].inUse && s_linkTxQueue[i].connHandle == connHandle)
            s_linkTxQueue[i].inUse = false;
}

void MESH_Maintenance(void)
{
    uint8_t i, pass;
    uint32_t now = xTaskGetTickCount();
    MeshConn_T *links = CONN_MGR_GetTable();

    /* Drain deferred GATT writes outside BLE receive callbacks. RGB entries
       are selected first and failed writes use exponential backoff. */
    for (pass = 0U; pass < 2U; pass++)
    {
        /* mesh_TrySendToConn already fails when the notification buffer is
           full, so the only reason to bound this is fairness. Keep the budget
           above one full broadcast fan-out or the queue overflows and drops. */
        uint8_t sentThisCycle = 0U;
        for (i = 0U; i < MESH_LINK_TX_QUEUE_SIZE && sentThisCycle < 8U; i++)
        {
            MeshHeader_T *queuedHdr;
            MeshConn_T *queuedConn;
            if (!s_linkTxQueue[i].inUse)
                continue;
            if ((now - s_linkTxQueue[i].enqueueTick) >= MESH_LINK_TX_MAX_AGE_TICKS)
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TX expired hdl=0x%04X cmd=0x%02X\r\n",
                    s_linkTxQueue[i].connHandle, s_linkTxQueue[i].packet[4]);
                s_linkTxQueue[i].inUse = false;
                continue;
            }
            if ((int32_t)(now - s_linkTxQueue[i].nextTick) < 0)
                continue;
            queuedHdr = (MeshHeader_T *)s_linkTxQueue[i].packet;
            if ((pass == 0U) != (queuedHdr->cmd == MESH_CMD_SET_DIMMER))
                continue;
            queuedConn = CONN_MGR_GetByHandle(s_linkTxQueue[i].connHandle);
            if (queuedConn == NULL)
            {
                s_linkTxQueue[i].inUse = false;
                continue;
            }
            if (mesh_TrySendToConn(queuedConn, s_linkTxQueue[i].packet,
                s_linkTxQueue[i].length))
            {
                s_linkTxQueue[i].inUse = false;
                sentThisCycle++;
            }
            else
            {
                if (s_linkTxQueue[i].attempts < 5U) s_linkTxQueue[i].attempts++;
                s_linkTxQueue[i].nextTick = now + pdMS_TO_TICKS(
                    100UL << s_linkTxQueue[i].attempts);
            }
        }
    }

    if (s_broadcastRetry.inUse &&
        (int32_t)(now - s_broadcastRetry.nextTick) >= 0)
    {
        MeshHeader_T *broadcastHdr = (MeshHeader_T *)s_broadcastRetry.packet;
        broadcastHdr->seq_num = s_seqNum++;
        broadcastHdr->ttl = MESH_MAX_TTL;
        mesh_AddToDupCache(s_myNodeId, broadcastHdr->seq_num);
        mesh_ForwardAll(s_broadcastRetry.packet, s_broadcastRetry.length, 0xFFFFU);
        if (--s_broadcastRetry.repeats == 0U)
            s_broadcastRetry.inUse = false;
        else
            s_broadcastRetry.nextTick = now + pdMS_TO_TICKS(800);
    }

    /* The CBFC downlink handshake takes several ATT round trips after GATT
       discovery ends, and can fail outright. Retry the JOIN with bounded
       exponential spacing: the first write the transport accepts is what
       marks this link ready and usable for forwarding. Deliberately does not
       require isReady, because that is the flag this loop establishes. */
    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        uint32_t joinDelay;
        if (!links[i].inUse || !links[i].isReady || links[i].topologyReady ||
            links[i].role != CONN_ROLE_CENTRAL || links[i].type != CONN_TYPE_LOCAL)
            continue;
        joinDelay = pdMS_TO_TICKS(1000UL <<
            ((links[i].joinRetries < 5U) ? links[i].joinRetries : 5U));
        if (links[i].lastJoinTick == 0U ||
            (now - links[i].lastJoinTick) >= joinDelay)
        {
            MESH_SendHello(links[i].connHandle);
            links[i].lastJoinTick = now;
            if (links[i].joinRetries < 5U) links[i].joinRetries++;
        }
    }
    /* At most one reliable retry per maintenance cycle. RGB is always
       selected before status/administrative traffic. */
    for (pass = 0U; pass < 2U; pass++)
    {
        for (i = 0U; i < MESH_TX_QUEUE_SIZE; i++)
        {
            MeshHeader_T *retryHdr;
            if (!s_pendingTx[i].inUse ||
                (int32_t)(now - s_pendingTx[i].nextRetryTick) < 0)
                continue;
            retryHdr = (MeshHeader_T *)s_pendingTx[i].packet;
            if ((pass == 0U) != (retryHdr->cmd == MESH_CMD_SET_DIMMER))
                continue;
            if (s_pendingTx[i].retries >= MESH_MAX_RETRIES)
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "ACK timeout dst=%u seq=%u\r\n",
                    retryHdr->dst_id, retryHdr->seq_num);
                s_pendingTx[i].inUse = false;
                continue;
            }
            mesh_ForwardAll(s_pendingTx[i].packet, s_pendingTx[i].length, 0xFFFFU);
            s_pendingTx[i].retries++;
            s_pendingTx[i].nextRetryTick = now +
                (MESH_ACK_TIMEOUT_TICKS << s_pendingTx[i].retries);
            pass = 2U;
            break;
        }
    }

    if ((int32_t)(now - s_nextHeartbeatTick) >= 0)
    {
        MeshConn_T *table = CONN_MGR_GetTable();
        s_nextHeartbeatTick = now + mesh_HeartbeatDelay();
        for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        {
            if (!table[i].inUse || !table[i].isReady || !table[i].topologyReady ||
                table[i].type != CONN_TYPE_LOCAL)
                continue;
            if (table[i].heartbeatAwaitingAck)
            {
                table[i].heartbeatMisses++;
                if (table[i].heartbeatMisses >= 3U)
                {
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                        "Heartbeat ACK timeout peer=%u hdl=0x%04X\r\n",
                        table[i].peerNodeId, table[i].connHandle);
                    (void)BLE_GAP_Disconnect(table[i].connHandle, 0x13);
                    continue;
                }
            }
            /* Central links that are not the primary parent are backup-parent
               links. They also need bidirectional presence traffic or the
               peer will incorrectly age them out after 90 seconds. */
            {
                uint8_t hb[4];
                hb[0] = s_rootId;
                hb[1] = s_depth;
                hb[2] = mesh_FreeSlots();
                hb[3] = ++table[i].heartbeatToken;
                if (mesh_SendDirect(table[i].connHandle, table[i].peerNodeId,
                    MESH_CMD_HEARTBEAT, hb, sizeof(hb)))
                    table[i].heartbeatAwaitingAck = true;
            }
            if ((now - table[i].lastActivityTick) >= MESH_LINK_DEAD_TICKS)
                (void)BLE_GAP_Disconnect(table[i].connHandle, 0x13);
        }
        mesh_UpdateAdvertisement();

        /* One line per heartbeat cycle describing how this node sees itself.
           When a node drops out of the mesh its own log is the only place
           that says whether it still had links, still had a parent, and what
           it was advertising as free capacity. */
        SYS_DEBUG_PRINT(SYS_ERROR_INFO,
            "STATUS id=%u links=%u root=%u depth=%u parent=%u free=%u %s\r\n",
            s_myNodeId, CONN_MGR_GetLocalLinkCount(), s_rootId, s_depth,
            s_parentId, mesh_FreeSlots(),
            s_networkFormed ? "FORMED" : "forming");
    }

    if ((now - s_lastFullReportTick) >= MESH_FULL_REPORT_TICKS)
    {
        s_lastFullReportTick = now;
        s_fullReportPending = true;
        mesh_MarkTopologyDirty();
    }

    if (s_topologyDirty && (int32_t)(now - s_nextReportTick) >= 0)
    {
        uint8_t nodes = 1U;
        uint8_t links = CONN_MGR_GetActiveCount();
        uint8_t report[7];
        MeshConn_T *parentConn;
        for (i = 0U; i < MESH_CHILD_SUMMARY_SIZE; i++)
        {
            if (s_childSummary[i].nodeId == 0U) continue;
            nodes = (uint8_t)((uint16_t)nodes + s_childSummary[i].nodes > 255U ?
                255U : nodes + s_childSummary[i].nodes);
            links = (uint8_t)((uint16_t)links + s_childSummary[i].links > 255U ?
                255U : links + s_childSummary[i].links);
        }
        report[0] = s_myNodeId;
        report[1] = s_parentId;
        report[2] = s_rootId;
        report[3] = s_depth;
        report[4] = nodes;
        report[5] = links;
        report[6] = s_fullReportPending ? 0x01U : 0x00U;
        parentConn = mesh_FindParentConnection();
        if (s_parentId == 0U)
        {
            s_topologyDirty = false; /* root has the aggregate locally */
            s_fullReportPending = false;
            s_reportRetryExp = 0U;
        }
        else if (parentConn != NULL && mesh_SendDirect(parentConn->connHandle,
            s_parentId, MESH_CMD_TOPOLOGY_REPORT, report, sizeof(report)))
        {
            s_topologyDirty = false;
            s_fullReportPending = false;
            s_reportRetryExp = 0U;
        }
        else
        {
            uint32_t backoffMs;
            if (s_reportRetryExp < 5U) s_reportRetryExp++;
            backoffMs = 500UL << s_reportRetryExp;
            s_nextReportTick = now + pdMS_TO_TICKS(backoffMs);
        }
    }

#if MESH_REBALANCE_ENABLE
    /* Cheap first: moving the parent pointer onto a link that already exists
       costs nothing, and often removes the need to release anything. */
    mesh_RebalanceOverExistingLinks(now);
    mesh_ReleaseRedundantUplink(now);
#endif

    /* Last, so it judges the state this cycle actually left behind. */
    mesh_UpdateNetworkFormed(now);
}

void MESH_SendClusterCommand(uint8_t clusterId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen)
{
    uint8_t clusterAddress;

    if (clusterId > GET_CLUSTER(NODE_ID_MAX))
        return;

    clusterAddress = MESH_CLUSTER_ADDR_BASE + clusterId;

    /* A cluster broadcast also applies to the originating node when it is a
       member of the selected cluster. It will not be executed again because
       the transmitted packet is entered in the duplicate cache. */
    if (GET_CLUSTER(s_myNodeId) == clusterId)
    {
        MeshHeader_T localHeader = {
            .dst_id = clusterAddress,
            .src_id = s_myNodeId,
            .seq_num = s_seqNum,
            .ttl = MESH_MAX_TTL,
            .cmd = cmd
        };
        mesh_ExecuteCommand(&localHeader, payload, payloadLen, 0xFFFF);
    }

    MESH_SendCommand(clusterAddress, cmd, payload, payloadLen);
}

uint8_t MESH_GetNodeId(void)
{
    return s_myNodeId;
}

void MESH_SendHello(uint16_t connHandle)
{
    MeshConn_T *conn = CONN_MGR_GetByHandle(connHandle);
    uint8_t join[3] = { s_myNodeId, s_rootId, s_depth };
    if (conn == NULL) return;
    /* Never queue the JOIN: MESH_Maintenance already retries it with
       exponential backoff while the link is not topologyReady. Queuing it
       would let it expire silently after marking the link joined. */
    if (mesh_SendDirectEx(connHandle, conn->peerNodeId, MESH_CMD_JOIN_REQUEST,
        join, sizeof(join), false))
    {
        /* A write that the GATT layer accepted is the only proof that the
           central->peripheral direction is actually open. BLE_TRSPC reports
           EVT_DISC_COMPLETE before it even starts the CBFC handshake, so
           trusting that event marks one-way links as usable and silently
           drops every packet forwarded into them. */
        CONN_MGR_SetReady(connHandle);
        /* Do not block data routing while JOIN_ACCEPT competes for a
           notification buffer inside the peer receive callback. */
        CONN_MGR_SetTopologyReady(connHandle);
        if (s_parentId == 0U && conn->peerNodeId != 0U &&
            conn->peerNodeId < s_myNodeId)
        {
            s_parentId = conn->peerNodeId;
            if (conn->peerNodeId < s_rootId) s_rootId = conn->peerNodeId;
            s_depth = 1U;
            mesh_UpdateAdvertisement();
            mesh_MarkTopologyDirty();
        }
    }
}

void MESH_OnLinkLost(uint8_t peerNodeId)
{
    uint8_t i;
    /* 0 means "peer id unknown": the phone, or a link whose JOIN never
       completed. s_parentId is also 0 when this node has no parent, so
       without this guard every such disconnect is mistaken for the loss of
       the parent, wiping the topology and flooding a mesh-wide
       REPAIR_REQUEST that nothing acts on. */
    if (peerNodeId == 0U)
    {
        mesh_MarkTopologyDirty();
        return;
    }
    for (i = 0U; i < MESH_CHILD_SUMMARY_SIZE; i++)
        if (s_childSummary[i].nodeId == peerNodeId)
            memset(&s_childSummary[i], 0, sizeof(s_childSummary[i]));
    if (peerNodeId == s_parentId)
    {
        s_parentId = 0U;
        s_rootId = s_myNodeId;
        s_depth = 0U;
        mesh_UpdateAdvertisement();
        /* Deliberately not a network-wide flood. The receiver does nothing
           with this beyond refreshing the link's activity timestamp, which
           MESH_ProcessIncoming already does for any packet, so every hop past
           the immediate neighbourhood is pure cost. It matters at 100 nodes:
           losing one node four levels up orphans its whole subtree at once,
           and each orphan flooding the entire network on the same tick is a
           broadcast storm at exactly the moment the mesh is already repairing. */
        mesh_SendCommandTtl(MESH_BROADCAST_ADDR, MESH_CMD_REPAIR_REQUEST,
            &s_myNodeId, 1U, 2U);
    }
    mesh_MarkTopologyDirty();
}

uint8_t MESH_GetRootId(void) { return s_rootId; }
uint8_t MESH_GetDepth(void) { return s_depth; }
void MESH_TopologyChanged(void)
{
    mesh_UpdateAdvertisement();
    mesh_MarkTopologyDirty();
}
