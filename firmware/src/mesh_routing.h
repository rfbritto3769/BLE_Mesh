#ifndef MESH_ROUTING_H
#define MESH_ROUTING_H

#include <stdint.h>
#include <stdbool.h>

#define MESH_CMD_SET_DIMMER     0x01
#define MESH_CMD_GET_STATUS     0x02
#define MESH_CMD_STATUS_RESP    0x03
#define MESH_CMD_IDENTIFY       0x04
#define MESH_CMD_PROVISION      0x05
#define MESH_CMD_UNPROVISION    0x06
#define MESH_CMD_ACK            0x07
#define MESH_CMD_HELLO          0x08
#define MESH_CMD_JOIN_REQUEST   0x09
#define MESH_CMD_JOIN_ACCEPT    0x0A
#define MESH_CMD_JOIN_REDIRECT  0x0B
#define MESH_CMD_HEARTBEAT      0x0C
#define MESH_CMD_TOPOLOGY_REPORT 0x0D
#define MESH_CMD_REPAIR_REQUEST 0x0E

#define MESH_BROADCAST_ADDR     0xFF
#define MESH_CLUSTER_ADDR_BASE  0xC0
#define MESH_CLUSTER_ADDR_MAX   0xC9
#define MESH_MAX_TTL            12
#define MESH_HEADER_SIZE        5
/* Replay window kept per source node, in packets. A single shared FIFO does
   not work here: one chatty neighbour evicts every other source's history, so
   the effective dedup window collapses to well under a second under load and
   delayed copies get re-executed and re-flooded. */
#define MESH_DUP_WINDOW         32U
#define MESH_MAX_PACKET_SIZE    20

typedef struct __attribute__((packed)) {
    uint8_t dst_id;
    uint8_t src_id;
    uint8_t seq_num;
    uint8_t ttl;
    uint8_t cmd;
} MeshHeader_T;

/* Anti-replay state for one source node: the highest sequence number seen and
   a bitmap of the MESH_DUP_WINDOW sequence numbers below it. */
typedef struct {
    uint32_t window;
    uint32_t tickStamp;
    uint8_t  lastSeq;
    bool     valid;
} MeshDupEntry_T;

void MESH_Init(uint8_t myNodeId);
void MESH_ProcessIncoming(uint16_t connHandle, uint16_t dataLen, uint8_t *p_data);
void MESH_SendCommand(uint8_t dstId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen);
void MESH_SendClusterCommand(uint8_t clusterId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen);
void MESH_Maintenance(void);
void MESH_SendHello(uint16_t connHandle);
void MESH_OnLinkLost(uint8_t peerNodeId);
void MESH_TopologyChanged(void);
uint8_t MESH_GetRootId(void);
uint8_t MESH_GetDepth(void);
uint8_t MESH_GetNodeId(void);

#endif
