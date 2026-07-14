#include <string.h>
#include "provisioning.h"
#include "peripheral/nvm/plib_nvm.h"
#include "definitions.h"

static ProvData_T s_provData;

void PROV_Init(void)
{
    volatile uint32_t *flashPtr = (volatile uint32_t *)PROV_STORAGE_ADDR;
    memcpy(&s_provData, (const void *)flashPtr, sizeof(ProvData_T));
}

uint8_t PROV_GetNodeId(void)
{
    if (s_provData.magic == PROV_MAGIC && s_provData.nodeId >= 1 && s_provData.nodeId <= 99)
        return (uint8_t)s_provData.nodeId;
    return 0;
}

bool PROV_IsProvisioned(void)
{
    return (PROV_GetNodeId() != 0);
}

bool PROV_SetNodeId(uint8_t nodeId)
{
    uint32_t writeWord;

    if (nodeId < 1 || nodeId > 99)
        return false;

    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "NVM erase @ 0x%08X\r\n", (unsigned int)PROV_STORAGE_ADDR);
    NVM_PageErase(PROV_STORAGE_ADDR);
    while (NVM_IsBusy()) {}

    if (NVM_ErrorGet() != 0)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "NVM erase ERR\r\n");
        return false;
    }

    writeWord = PROV_MAGIC;
    NVM_WordWrite(writeWord, PROV_STORAGE_ADDR);
    while (NVM_IsBusy()) {}

    writeWord = (uint32_t)nodeId;
    NVM_WordWrite(writeWord, PROV_STORAGE_ADDR + 4U);
    while (NVM_IsBusy()) {}

    if (NVM_ErrorGet() != 0)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "NVM write ERR\r\n");
        return false;
    }

    volatile uint32_t *verifyMagic = (volatile uint32_t *)PROV_STORAGE_ADDR;
    volatile uint32_t *verifyId = (volatile uint32_t *)(PROV_STORAGE_ADDR + 4U);
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "NVM verify: magic=%08X id=%d\r\n",
        (unsigned int)*verifyMagic, (int)*verifyId);

    if (*verifyMagic == PROV_MAGIC && (uint8_t)(*verifyId) == nodeId)
    {
        s_provData.magic = PROV_MAGIC;
        s_provData.nodeId = nodeId;
        return true;
    }

    return false;
}

void PROV_Reset(void)
{
    NVM_PageErase(PROV_STORAGE_ADDR);
    while (NVM_IsBusy()) {}

    s_provData.magic = 0xFFFFFFFF;
    s_provData.nodeId = 0xFF;
}
