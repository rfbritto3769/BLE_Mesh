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

#define MESH_BROADCAST_ADDR     0xFF
#define MESH_CLUSTER_ADDR_BASE  0xC0
#define MESH_CLUSTER_ADDR_MAX   0xC9
#define MESH_MAX_TTL            5
#define MESH_HEADER_SIZE        5
#define MESH_DUP_CACHE_SIZE     32
#define MESH_MAX_PACKET_SIZE    20

typedef struct __attribute__((packed)) {
    uint8_t dst_id;
    uint8_t src_id;
    uint8_t seq_num;
    uint8_t ttl;
    uint8_t cmd;
} MeshHeader_T;

typedef struct {
    uint8_t src_id;
    uint8_t seq_num;
    uint32_t tickStamp;
} MeshDupEntry_T;

void MESH_Init(uint8_t myNodeId);
void MESH_ProcessIncoming(uint16_t connHandle, uint16_t dataLen, uint8_t *p_data);
void MESH_SendCommand(uint8_t dstId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen);
void MESH_SendClusterCommand(uint8_t clusterId, uint8_t cmd, uint8_t *payload, uint8_t payloadLen);
uint8_t MESH_GetNodeId(void);

#endif
