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
    /* True when the remote address was already known from a DIMMER_xx
       advertising report, i.e. this really is a mesh node. Set at connect time,
       before the peer has said anything, which is the only moment where a mesh
       child and the phone/GUI can still be told apart: everything else (type,
       peerNodeId) only becomes known once the peer sends its first packet.
       Used to keep one peripheral slot free for the app and to keep an idle
       app link from gating mesh discovery. */
    bool meshPeer;
    uint32_t createdTick;
    uint32_t lastActivityTick;
    int8_t rssi;
    uint32_t lastJoinTick;
    uint8_t joinRetries;
    uint8_t heartbeatToken;
    uint8_t heartbeatMisses;
    bool heartbeatAwaitingAck;
    /* Where this neighbour sits in the tree, as it last reported. Heartbeats
       already carry root, depth and free slots on every link; keeping them
       lets a node judge its own placement against every peer it is already
       talking to, with no scan and no extra traffic. peerInfoTick is 0 until
       the first report arrives - a peer of unknown depth must never be
       treated as an improvement. */
    uint8_t peerRootId;
    uint8_t peerDepth;
    uint8_t peerFreeSlots;
    uint32_t peerInfoTick;
} MeshConn_T;

void CONN_MGR_Init(void);
bool CONN_MGR_AddConnection(uint16_t connHandle, ConnRole_T role, bool meshPeer);
void CONN_MGR_RemoveConnection(uint16_t connHandle);
void CONN_MGR_SetPeerNodeId(uint16_t connHandle, uint8_t nodeId);
void CONN_MGR_SetReady(uint16_t connHandle);
void CONN_MGR_SetTopologyReady(uint16_t connHandle);
void CONN_MGR_SetType(uint16_t connHandle, ConnType_T type);
/* Promotes a link to "known mesh node" once the peer has identified itself.
   The connect-time guess cannot be trusted on a node that already stopped
   scanning: it never heard the newcomer advertise. */
void CONN_MGR_SetMeshPeer(uint16_t connHandle, bool meshPeer);
/* Records a neighbour's own view of its place in the tree. freeSlots is
   passed as 0xFF when the carrying message does not include it. */
void CONN_MGR_SetPeerTopology(uint16_t connHandle, uint8_t rootId,
    uint8_t depth, uint8_t freeSlots);
MeshConn_T* CONN_MGR_GetByHandle(uint16_t connHandle);
uint8_t CONN_MGR_GetCentralCount(void);
uint8_t CONN_MGR_GetPeripheralCount(void);
uint8_t CONN_MGR_GetActiveCount(void);
uint8_t CONN_MGR_GetMeshPeripheralCount(void);
uint8_t CONN_MGR_GetMeshChildCount(void);
uint8_t CONN_MGR_GetLocalLinkCount(void);
uint8_t CONN_MGR_GetReadyLocalLinkCount(void);
void CONN_MGR_Touch(uint16_t connHandle);
void CONN_MGR_SetRssi(uint16_t connHandle, int8_t rssi);
bool CONN_MGR_IsConnectedToPeer(uint8_t nodeId);
bool CONN_MGR_HasUnreadyCentral(void);
bool CONN_MGR_HasUnreadyMeshLink(void);
MeshConn_T* CONN_MGR_GetTable(void);
void CONN_MGR_SweepStale(uint32_t maxAgeTicks);

#endif
