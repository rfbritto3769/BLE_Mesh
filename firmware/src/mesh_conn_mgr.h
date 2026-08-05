#ifndef MESH_CONN_MGR_H
#define MESH_CONN_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_MAX_CONNECTIONS    6
/* Uplinks a node opens vs downlinks it offers. Because a node only ever
   connects to lower node ids, the aggregate demand is MESH_MAX_CENTRAL*(N-1)
   and the aggregate supply is MESH_MAX_MESH_CHILDREN*N. With 3 uplinks and 2
   children the demand exceeds the supply and the highest node ids end up with
   no parent at all - they see every candidate as "no free slot" and never
   receive a broadcast. Extra uplinks buy nothing in a flood mesh anyway, and
   fewer links per node also cuts the radio contention behind the 0x08
   supervision timeouts. */
#define MESH_MAX_CENTRAL        2
#define MESH_MAX_PERIPHERAL     3

/* Mesh children accepted as peripheral. One peripheral slot is kept free for
   the phone/GUI. This is the value advertised as "free slots" and the value
   enforced when a JOIN_REQUEST is admitted; the two must agree or peers keep
   connecting to full nodes and being kicked straight back out. */
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
MeshConn_T* CONN_MGR_GetTable(void);
void CONN_MGR_SweepStale(uint32_t maxAgeTicks);

#endif
