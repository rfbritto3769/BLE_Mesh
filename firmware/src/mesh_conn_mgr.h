#ifndef MESH_CONN_MGR_H
#define MESH_CONN_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_MAX_CONNECTIONS    6
/* Stable tree: one parent (central role), up to four mesh children and one
   peripheral slot reserved for the phone. This stays within the six-link
   controller limit without simultaneous multi-parent connection storms. */
#define MESH_MAX_CENTRAL        1
#define MESH_MAX_PERIPHERAL     5

/* Mesh children accepted as peripheral. One peripheral slot is kept free for
   the phone/GUI. This is the value advertised as "free slots" and the value
   enforced when a JOIN_REQUEST is admitted; the two must agree or peers keep
   connecting to full nodes and being kicked straight back out.
 *
 * One peripheral slot is reserved for the phone, leaving four mesh children. */
#define MESH_MAX_MESH_CHILDREN  (MESH_MAX_PERIPHERAL - 1)

typedef enum {
    CONN_ROLE_NONE = 0,
    CONN_ROLE_CENTRAL,
    CONN_ROLE_PERIPHERAL
} ConnRole_T;

typedef enum {
    CONN_TYPE_UNKNOWN = 0,
    CONN_TYPE_PHONE,
    CONN_TYPE_LOCAL,
    CONN_TYPE_GATEWAY
} ConnType_T;

typedef struct {
    uint16_t connHandle;
    ConnRole_T role;
    ConnType_T type;
    uint8_t peerNodeId;
    bool isReady;
    bool topologyReady;
    bool inUse;
    uint32_t createdTick;
    uint32_t lastActivityTick;
    int8_t rssi;
    uint32_t lastJoinTick;
    uint8_t joinRetries;
    uint8_t heartbeatToken;
    uint8_t heartbeatMisses;
    bool heartbeatAwaitingAck;
} MeshConn_T;

void CONN_MGR_Init(void);
bool CONN_MGR_AddConnection(uint16_t connHandle, ConnRole_T role);
void CONN_MGR_RemoveConnection(uint16_t connHandle);
void CONN_MGR_SetPeerNodeId(uint16_t connHandle, uint8_t nodeId);
void CONN_MGR_SetReady(uint16_t connHandle);
void CONN_MGR_SetTopologyReady(uint16_t connHandle);
void CONN_MGR_SetType(uint16_t connHandle, ConnType_T type);
MeshConn_T* CONN_MGR_GetByHandle(uint16_t connHandle);
uint8_t CONN_MGR_GetCentralCount(void);
uint8_t CONN_MGR_GetPeripheralCount(void);
uint8_t CONN_MGR_GetActiveCount(void);
uint8_t CONN_MGR_GetMeshPeripheralCount(void);
uint8_t CONN_MGR_GetLocalLinkCount(void);
void CONN_MGR_Touch(uint16_t connHandle);
void CONN_MGR_SetRssi(uint16_t connHandle, int8_t rssi);
bool CONN_MGR_IsConnectedToPeer(uint8_t nodeId);
bool CONN_MGR_HasUnreadyCentral(void);
bool CONN_MGR_HasUnreadyMeshLink(void);
MeshConn_T* CONN_MGR_GetTable(void);
void CONN_MGR_SweepStale(uint32_t maxAgeTicks);

#endif
