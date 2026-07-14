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

static uint8_t s_myNodeId;
static uint8_t s_seqNum;
static MeshDupEntry_T s_dupCache[MESH_DUP_CACHE_SIZE];
static uint8_t s_dupIndex;

static uint16_t s_lastPhoneConnHandle = 0xFFFF;

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
        if (!table[i].inUse || !table[i].isReady) continue;
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
        if (!table[i].inUse || !table[i].isReady) continue;
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
        if (!table[i].inUse || !table[i].isReady) continue;
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
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!table[i].inUse) continue;
        if (!table[i].isReady)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "FWD skip hdl=0x%04X not ready\r\n", table[i].connHandle);
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

static void mesh_ExecuteCommand(MeshHeader_T *pHdr, uint8_t *payload, uint8_t payloadLen, uint16_t connHandle)
{
    switch (pHdr->cmd)
    {
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
            MESH_SendCommand(pHdr->src_id, MESH_CMD_STATUS_RESP, respPayload, 4);
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
                if (PROV_SetNodeId(newId))
                {
                    LED_Dimmer_SetRGB(0, 100, 0);
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Stored. Rebooting...\r\n");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    NVIC_SystemReset();
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
}

void MESH_ProcessIncoming(uint16_t connHandle, uint16_t dataLen, uint8_t *p_data)
{
    MeshHeader_T *pHdr;
    uint8_t *payload;
    uint8_t payloadLen;

    if (dataLen < MESH_HEADER_SIZE)
        return;

    pHdr = (MeshHeader_T *)p_data;
    payload = p_data + MESH_HEADER_SIZE;
    payloadLen = (uint8_t)(dataLen - MESH_HEADER_SIZE);

    if (pHdr->ttl == 0)
        return;

    if (pHdr->src_id == NODE_ID_PHONE)
    {
        s_lastPhoneConnHandle = connHandle;
        pHdr->src_id = s_myNodeId;
        pHdr->seq_num = s_seqNum++;
        mesh_AddToDupCache(s_myNodeId, pHdr->seq_num);
    }
    else
    {
        if (mesh_IsDuplicate(pHdr->src_id, pHdr->seq_num))
            return;
        mesh_AddToDupCache(pHdr->src_id, pHdr->seq_num);
    }

    uint8_t dstId = pHdr->dst_id;

    if (dstId == s_myNodeId || dstId == MESH_BROADCAST_ADDR ||
        dstId == NODE_ID_PHONE || mesh_IsMyCluster(dstId))
    {
        mesh_ExecuteCommand(pHdr, payload, payloadLen, connHandle);
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
