#ifndef MESH_CONN_MGR_H
#define MESH_CONN_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_MAX_CONNECTIONS    6
#define MESH_MAX_CENTRAL        3
#define MESH_MAX_PERIPHERAL     3

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
    bool inUse;
    uint32_t createdTick;
} MeshConn_T;

void CONN_MGR_Init(void);
bool CONN_MGR_AddConnection(uint16_t connHandle, ConnRole_T role);
void CONN_MGR_RemoveConnection(uint16_t connHandle);
void CONN_MGR_SetPeerNodeId(uint16_t connHandle, uint8_t nodeId);
void CONN_MGR_SetReady(uint16_t connHandle);
void CONN_MGR_SetType(uint16_t connHandle, ConnType_T type);
MeshConn_T* CONN_MGR_GetByHandle(uint16_t connHandle);
uint8_t CONN_MGR_GetCentralCount(void);
uint8_t CONN_MGR_GetPeripheralCount(void);
uint8_t CONN_MGR_GetActiveCount(void);
bool CONN_MGR_IsConnectedToPeer(uint8_t nodeId);
MeshConn_T* CONN_MGR_GetTable(void);
void CONN_MGR_SweepStale(uint32_t maxAgeTicks);

#endif
