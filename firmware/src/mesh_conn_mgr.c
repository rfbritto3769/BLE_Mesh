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

bool CONN_MGR_AddConnection(uint16_t connHandle, ConnRole_T role, bool meshPeer)
{
    uint8_t i;

    if (role == CONN_ROLE_CENTRAL && CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
        return false;
    if (role == CONN_ROLE_PERIPHERAL)
    {
        if (CONN_MGR_GetPeripheralCount() >= MESH_MAX_PERIPHERAL)
            return false;
        /* Mesh children are capped one below the peripheral limit so the last
           slot always stays available to the phone/GUI. The JOIN handler also
           enforces MESH_MAX_MESH_CHILDREN, but it only runs once the peer has
           identified itself: between CONNECTED and JOIN a mesh child counts as
           nothing, so five of them could take every peripheral slot and the
           app was answered with "AddConn FULL" and reason 0x13 while the mesh
           was still forming. */
        if (meshPeer && CONN_MGR_GetMeshChildCount() >= MESH_MAX_MESH_CHILDREN)
            return false;
    }

    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!s_connTable[i].inUse)
        {
            s_connTable[i].connHandle = connHandle;
            s_connTable[i].role = role;
            s_connTable[i].peerNodeId = 0;
            s_connTable[i].isReady = false;
            s_connTable[i].topologyReady = false;
            s_connTable[i].meshPeer = meshPeer;
            s_connTable[i].inUse = true;
            s_connTable[i].createdTick = xTaskGetTickCount();
            s_connTable[i].lastActivityTick = s_connTable[i].createdTick;
            s_connTable[i].rssi = -127;
            s_connTable[i].lastJoinTick = 0U;
            s_connTable[i].joinRetries = 0U;
            s_connTable[i].heartbeatToken = 0U;
            s_connTable[i].heartbeatMisses = 0U;
            s_connTable[i].heartbeatAwaitingAck = false;
            return true;
        }
    }
    return false;
}

/* Clears every entry carrying this handle. Stopping at the first match would
   leak a slot for good if the table ever ended up with a duplicate, and a
   leaked slot silently blocks all further connections. */
void CONN_MGR_RemoveConnection(uint16_t connHandle)
{
    uint8_t i;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse && s_connTable[i].connHandle == connHandle)
            memset(&s_connTable[i], 0, sizeof(MeshConn_T));
    }
}

uint8_t CONN_MGR_GetLocalLinkCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        if (s_connTable[i].inUse && s_connTable[i].type == CONN_TYPE_LOCAL)
            count++;
    return count;
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

void CONN_MGR_SetTopologyReady(uint16_t connHandle)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p)
    {
        p->topologyReady = true;
        p->joinRetries = 0U;
        p->lastActivityTick = xTaskGetTickCount();
    }
}

void CONN_MGR_Touch(uint16_t connHandle)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p) p->lastActivityTick = xTaskGetTickCount();
}

void CONN_MGR_SetRssi(uint16_t connHandle, int8_t rssi)
{
    MeshConn_T *p = CONN_MGR_GetByHandle(connHandle);
    if (p) p->rssi = rssi;
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

uint8_t CONN_MGR_GetMeshPeripheralCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        if (s_connTable[i].inUse && s_connTable[i].role == CONN_ROLE_PERIPHERAL &&
            s_connTable[i].type == CONN_TYPE_LOCAL) count++;
    return count;
}

/* Peripheral links that are known to be mesh nodes, whether or not they have
   joined yet. Unlike CONN_MGR_GetMeshPeripheralCount this already counts a
   child that has connected but not yet sent its JOIN, which is what makes the
   reserved phone slot hold during mesh formation. */
uint8_t CONN_MGR_GetMeshChildCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        if (s_connTable[i].inUse && s_connTable[i].role == CONN_ROLE_PERIPHERAL &&
            s_connTable[i].meshPeer && s_connTable[i].type != CONN_TYPE_PHONE) count++;
    return count;
}

/* Mesh links that are fully usable: transport open and admitted into the tree.
   This is the count the "network formed" indicator is built on - a link that
   is merely connected forwards nothing. */
uint8_t CONN_MGR_GetReadyLocalLinkCount(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
        if (s_connTable[i].inUse && s_connTable[i].type == CONN_TYPE_LOCAL &&
            s_connTable[i].isReady && s_connTable[i].topologyReady) count++;
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

bool CONN_MGR_HasUnreadyCentral(void)
{
    uint8_t i;
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (s_connTable[i].inUse &&
            s_connTable[i].role == CONN_ROLE_CENTRAL &&
            !s_connTable[i].topologyReady)
            return true;
    }
    return false;
}

bool CONN_MGR_HasUnreadyMeshLink(void)
{
    uint8_t i;
    for (i = 0U; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!s_connTable[i].inUse || s_connTable[i].type == CONN_TYPE_PHONE)
            continue;
        /* A peripheral that never advertised as a mesh node is the phone, and
           it is CONN_TYPE_UNKNOWN until its first packet. Counting that window
           as an unfinished mesh link froze all discovery and reconnection for
           as long as the app stayed connected and idle - the mesh simply never
           formed while the GUI was open. Only links this node is establishing
           itself (central role) or already knows to be mesh links may gate. */
        if (s_connTable[i].role == CONN_ROLE_PERIPHERAL && !s_connTable[i].meshPeer)
            continue;
        if (!s_connTable[i].isReady ||
            (s_connTable[i].type == CONN_TYPE_LOCAL &&
             !s_connTable[i].topologyReady))
            return true;
    }
    return false;
}

MeshConn_T* CONN_MGR_GetTable(void)
{
    return s_connTable;
}

/* Drops links that never became usable. "Usable" is deliberately judged by
   traffic, not only by isReady: isReady is set from BLE_TRSPS_EVT_TX_STATUS,
   which only fires when the peer subscribes to the TRSP TX characteristic. An
   app that writes commands but never enables notifications was therefore
   disconnected on the dot at maxAgeTicks even while it was actively driving
   the lights, because receiving data only refreshes lastActivityTick. */
void CONN_MGR_SweepStale(uint32_t maxAgeTicks)
{
    uint8_t i;
    uint32_t now = xTaskGetTickCount();
    for (i = 0; i < MESH_MAX_CONNECTIONS; i++)
    {
        if (!s_connTable[i].inUse || s_connTable[i].isReady)
            continue;
        /* An identified app link is never swept. It has no JOIN to complete,
           so there is no state it could still be waiting to reach. */
        if (s_connTable[i].type == CONN_TYPE_PHONE)
            continue;
        if ((now - s_connTable[i].createdTick) <= maxAgeTicks)
            continue;
        if ((now - s_connTable[i].lastActivityTick) <= maxAgeTicks)
            continue;

        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Sweep stale hdl=0x%04X\r\n",
            s_connTable[i].connHandle);
        BLE_GAP_Disconnect(s_connTable[i].connHandle, 0x13);
        memset(&s_connTable[i], 0, sizeof(MeshConn_T));
    }
}
