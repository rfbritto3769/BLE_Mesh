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
static MeshDupEntry_T s_dupCache[MESH_DUP_CACHE_SIZE];
static uint8_t s_dupIndex;

static uint16_t s_lastPhoneConnHandle = 0xFFFF;

#define MESH_TX_QUEUE_SIZE       8U
#define MESH_ACK_TIMEOUT_TICKS   pdMS_TO_TICKS(700)
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
#define MESH_LINK_TX_QUEUE_SIZE 16U
typedef struct {
    bool inUse;
    uint16_t connHandle;
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t attempts;
    uint32_t nextTick;
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
#define MESH_HEARTBEAT_BASE_MS  30000UL
#define MESH_HEARTBEAT_JITTER_MS 5000UL
#define MESH_LINK_DEAD_TICKS     pdMS_TO_TICKS(90000)
#define MESH_FULL_REPORT_TICKS   pdMS_TO_TICKS(300000)
#define MESH_REPORT_DEBOUNCE_MS  2000UL
#define MESH_CHILD_SUMMARY_SIZE  3U
typedef struct {
    uint8_t nodeId;
    uint8_t nodes;
    uint8_t links;
} MeshChildSummary_T;
static MeshChildSummary_T s_childSummary[MESH_CHILD_SUMMARY_SIZE];

static bool mesh_SendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen);
static bool mesh_TrySendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen);

/* A scanning peer connects to us as central, so it consumes one of our
   peripheral slots. Advertise the admission capacity actually enforced in the
   JOIN_REQUEST handler, otherwise peers keep connecting to full nodes. */
static uint8_t mesh_FreeSlots(void)
{
    uint8_t used = CONN_MGR_GetMeshPeripheralCount();
    return (used < MESH_MAX_MESH_CHILDREN) ?
        (uint8_t)(MESH_MAX_MESH_CHILDREN - used) : 0U;
}

static void mesh_UpdateAdvertisement(void)
{
    APP_BLE_UpdateTopologyAdvertisement(s_rootId, s_depth, mesh_FreeSlots(),
        (uint8_t)(0x01U | ((s_parentId != 0U || s_rootId == s_myNodeId) ? 0x02U : 0U)));
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

static bool mesh_SendDirect(uint16_t connHandle, uint8_t dstId, uint8_t cmd,
    const uint8_t *payload, uint8_t payloadLen)
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
    return mesh_SendToConn(conn, packet, MESH_HEADER_SIZE + payloadLen);
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

static bool mesh_IsDuplicate(uint8_t srcId, uint8_t seqNum)
{
    uint8_t i;
    uint32_t now = xTaskGetTickCount();
    for (i = 0; i < MESH_DUP_CACHE_SIZE; i++)
    {
        if (s_dupCache[i].src_id == srcId && s_dupCache[i].seq_num == seqNum)
        {
            if ((now - s_dupCache[i].tickStamp) < MESH_DUP_MAX_AGE_TICKS)
                return true;
            s_dupCache[i].src_id = 0;
            return false;
        }
    }
    return false;
}

static void mesh_AddToDupCache(uint8_t srcId, uint8_t seqNum)
{
    s_dupCache[s_dupIndex].src_id = srcId;
    s_dupCache[s_dupIndex].seq_num = seqNum;
    s_dupCache[s_dupIndex].tickStamp = xTaskGetTickCount();
    s_dupIndex = (s_dupIndex + 1) % MESH_DUP_CACHE_SIZE;
}

static bool mesh_SendToConn(MeshConn_T *conn, uint8_t *p_packet, uint16_t packetLen)
{
    uint8_t i;
    if (mesh_TrySendToConn(conn, p_packet, packetLen))
        return true;

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

static void mesh_ExecuteCommand(MeshHeader_T *pHdr, uint8_t *payload, uint8_t payloadLen, uint16_t connHandle)
{
    switch (pHdr->cmd)
    {
        case MESH_CMD_HELLO:
            if (payloadLen >= 1U)
            {
                CONN_MGR_SetPeerNodeId(connHandle, payload[0]);
                CONN_MGR_SetType(connHandle, CONN_TYPE_LOCAL);
                CONN_MGR_SetReady(connHandle);
                CONN_MGR_SetTopologyReady(connHandle);
            }
            break;

        case MESH_CMD_JOIN_REQUEST:
            if (payloadLen >= 3U)
            {
                uint8_t reply[3];
                CONN_MGR_SetPeerNodeId(connHandle, payload[0]);
                CONN_MGR_SetType(connHandle, CONN_TYPE_LOCAL);
                CONN_MGR_SetReady(connHandle);
                /* SetType(LOCAL) above already counts this connection. */
                if (CONN_MGR_GetMeshPeripheralCount() > MESH_MAX_MESH_CHILDREN)
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
                }
                if (payload[0] < s_rootId || s_parentId == 0U)
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
            if (payloadLen >= 2U && pHdr->src_id == s_parentId &&
                (payload[0] != s_rootId || (uint8_t)(payload[1] + 1U) != s_depth))
            {
                s_rootId = payload[0];
                s_depth = (uint8_t)(payload[1] + 1U);
                mesh_UpdateAdvertisement();
                mesh_MarkTopologyDirty();
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
    s_dupIndex = 0;
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
        if ((pHdr->dst_id == MESH_BROADCAST_ADDR ||
             mesh_IsClusterAddress(pHdr->dst_id)) &&
            pHdr->cmd == MESH_CMD_SET_DIMMER && dataLen <= MESH_MAX_PACKET_SIZE)
        {
            memcpy(s_broadcastRetry.packet, p_data, dataLen);
            s_broadcastRetry.length = (uint8_t)dataLen;
            s_broadcastRetry.repeats = 2U;
            s_broadcastRetry.nextTick = xTaskGetTickCount() + pdMS_TO_TICKS(400);
            s_broadcastRetry.inUse = true;
        }
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

void MESH_SendCommand(uint8_t dstId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen)
{
    uint8_t packet[MESH_MAX_PACKET_SIZE];
    MeshHeader_T *pHdr = (MeshHeader_T *)packet;
    uint16_t totalLen;

    if (payloadLen > (MESH_MAX_PACKET_SIZE - MESH_HEADER_SIZE))
        payloadLen = MESH_MAX_PACKET_SIZE - MESH_HEADER_SIZE;

    pHdr->dst_id = dstId;
    pHdr->src_id = s_myNodeId;
    pHdr->seq_num = s_seqNum++;
    pHdr->ttl = MESH_MAX_TTL;
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
            if (!s_linkTxQueue[i].inUse ||
                (int32_t)(now - s_linkTxQueue[i].nextTick) < 0)
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

    /* GATT discovery and CCC configuration can finish before the peer is able
       to consume the first JOIN. Repeat it with bounded exponential spacing
       until JOIN_ACCEPT confirms that routing is available in both directions. */
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
        uint8_t hb[3] = { s_rootId, s_depth, mesh_FreeSlots() };
        MeshConn_T *table = CONN_MGR_GetTable();
        s_nextHeartbeatTick = now + mesh_HeartbeatDelay();
        for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        {
            if (!table[i].inUse || !table[i].isReady || !table[i].topologyReady ||
                table[i].type != CONN_TYPE_LOCAL)
                continue;
            /* Central links that are not the primary parent are backup-parent
               links. They also need bidirectional presence traffic or the
               peer will incorrectly age them out after 90 seconds. */
            (void)mesh_SendDirect(table[i].connHandle, table[i].peerNodeId,
                MESH_CMD_HEARTBEAT, hb, sizeof(hb));
            if ((now - table[i].lastActivityTick) >= MESH_LINK_DEAD_TICKS)
                (void)BLE_GAP_Disconnect(table[i].connHandle, 0x13);
        }
        mesh_UpdateAdvertisement();
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
    if (mesh_SendDirect(connHandle, conn->peerNodeId, MESH_CMD_JOIN_REQUEST,
        join, sizeof(join)))
    {
        /* A successful write queues JOIN_REQUEST on the established GATT
           link. Do not block data routing while JOIN_ACCEPT competes for a
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
    for (i = 0U; i < MESH_CHILD_SUMMARY_SIZE; i++)
        if (s_childSummary[i].nodeId == peerNodeId)
            memset(&s_childSummary[i], 0, sizeof(s_childSummary[i]));
    if (peerNodeId == s_parentId)
    {
        s_parentId = 0U;
        s_rootId = s_myNodeId;
        s_depth = 0U;
        mesh_UpdateAdvertisement();
        MESH_SendCommand(MESH_BROADCAST_ADDR, MESH_CMD_REPAIR_REQUEST,
            &s_myNodeId, 1U);
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
