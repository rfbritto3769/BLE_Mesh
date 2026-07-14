#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <stdint.h>
#include <stdbool.h>

#define PROV_MAGIC          0xD1A7E501
#define PROV_STORAGE_ADDR   0x010F0000

typedef struct {
    uint32_t magic;
    uint32_t nodeId;
} ProvData_T;

void PROV_Init(void);
uint8_t PROV_GetNodeId(void);
bool PROV_IsProvisioned(void);
bool PROV_SetNodeId(uint8_t nodeId);
void PROV_Reset(void);

#endif
