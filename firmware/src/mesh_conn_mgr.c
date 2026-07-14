#include <string.h>
#include "mesh_conn_mgr.h"
#include "ble_gap.h"
#include "FreeRTOS.h"
#include "task.h"
#include "definitions.h"

static MeshConn_T s_connTable[MESH_MAX_CONNECTIONS];

void CONN_MGR_Init(void)
{
    memset(s_connTable, 0, sizeof(s_connTable));
}

bool CONN_MGR_AddConnection(uint16_t connHandle, ConnRole_T role)
{
    uint8_t i;

    if (role == CONN_ROLE_CENTRAL && CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
        return false;
    if (role == CONN_ROLE_PERIPHERAL && CONN_MGR_GetPeripheralCount() >= MESH_MAX_PERIPHERAL)
        return false;

    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!s_connTable[i].inUse)
        {
            s_connTable[i].connHandle = connHandle;
            s_connTable[i].role = role;
            s_connTable[i].peerNodeId = 0;
            s_connTable[i].isReady = false;
            s_connTable[i].inUse = true;
            s_connTable[i].createdTick = xTaskGetTickCount();
            return true;
        }
    }
    return false;
}

void CONN_MGR_RemoveConnection(uint16_t connHandle)
{
    uint8_t i;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].connHandle == connHandle)
        {
            memset(&s_connTable[i], 0, sizeof(MeshConn_T));
            return;
        }
    }
}

void CONN_MGR_SetPeerNodeId(uint16_t connHandle, uint8_t nodeId)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p) p->peerNodeId = nodeId;
}

void CONN_MGR_SetReady(uint16_t connHandle)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p) p->isReady = true;
}

void CONN_MGR_SetType(uint16_t connHandle, ConnType_T type)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p) p->type = type;
}

MeshConn_T* CONN_MGR_GetByHandle(uint16_t connHandle)
{
    uint8_t i;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].connHandle == connHandle)
            return &s_connTable[i];
    }
    return NULL;
}

uint8_t CONN_MGR_GetCentralCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].role == CONN_ROLE_CENTRAL)
            count++;
    }
    return count;
}

uint8_t CONN_MGR_GetPeripheralCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].role == CONN_ROLE_PERIPHERAL)
            count++;
    }
    return count;
}

uint8_t CONN_MGR_GetActiveCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse)
            count++;
    }
    return count;
}

bool CONN_MGR_IsConnectedToPeer(uint8_t nodeId)
{
    uint8_t i;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].peerNodeId == nodeId)
            return true;
    }
    return false;
}

MeshConn_T* CONN_MGR_GetTable(void)
{
    return s_connTable;
}

void CONN_MGR_SweepStale(uint32_t maxAgeTicks)
{
    uint8_t i;
    uint32_t now = xTaskGetTickCount();
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && !s_connTable[i].isReady)
        {
            if ((now - s_connTable[i].createdTick) > maxAgeTicks)
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Sweep stale hdl=0x%04X\r\n",
                    s_connTable[i].connHandle);
                BLE_GAP_Disconnect(s_connTable[i].connHandle, 0x13);
                memset(&s_connTable[i], 0, sizeof(MeshConn_T));
            }
        }
    }
}
