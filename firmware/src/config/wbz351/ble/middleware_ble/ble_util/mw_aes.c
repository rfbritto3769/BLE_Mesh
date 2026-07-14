/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

/*******************************************************************************
  Middleware AES Source File

  Company:
    Microchip Technology Inc.

  File Name:
    mw_aes.c

  Summary:
    This file contains the Middleware AES functions for application user.

  Description:
    This file contains the Middleware AES functions for application user.
 *******************************************************************************/


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "mba_error_defs.h"
#include "mw_aes.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

uint16_t MW_AES_CbcDecryptInit(MW_AES_Ctx_T * p_ctx, uint8_t *p_aesKey, uint8_t *p_iv)
{
    uint16_t result;

    SX_CLK_ENABLE();
    
    p_ctx->aesKeyRef=SX_KEYREF_LOAD_MATERIAL(16, (char *)p_aesKey);
    if (SX_OK!=SX_BLKCIPHER_CREATE_AESCBC_DEC(&p_ctx->aesBlkCipher, &p_ctx->aesKeyRef, (char *)p_iv))
    {
        result = MBA_RES_FAIL;
    }
    else
    {
        result = MBA_RES_SUCCESS;
    }
    
    SX_CLK_DISABLE();

    return result;

}

uint16_t MW_AES_AesCbcDecrypt(MW_AES_Ctx_T * p_ctx, uint16_t length, uint8_t *p_plainText, uint8_t *p_chiperText)
{
    int32_t s;

    SX_CLK_ENABLE();

    s = SX_BLKCIPHER_CRYPT(&p_ctx->aesBlkCipher, (char *)p_chiperText, length, (char *)p_plainText);
    if (s == SX_OK)
    {
        s = SX_BLKCIPHER_SAVE_STATE(&p_ctx->aesBlkCipher);
    }
    if (s == SX_OK)
    {
    s = SX_BLKCIPHER_WAIT(&p_ctx->aesBlkCipher);
    }
    if (s == SX_OK)
    {
    s = SX_BLKCIPHER_RESUME_STATE(&p_ctx->aesBlkCipher);
    }

    SX_CLK_DISABLE();


    if (s != SX_OK)
    {
        return MBA_RES_FAIL;
    }

    return MBA_RES_SUCCESS;
}

uint16_t MW_AES_EcbEncryptInit(MW_AES_Ctx_T * p_ctx, uint8_t *p_aesKey)
{
    uint16_t result;

    SX_CLK_ENABLE();
    
    p_ctx->aesKeyRef=SX_KEYREF_LOAD_MATERIAL(16, (char *)p_aesKey);
    if (SX_OK!=SX_BLKCIPHER_CREATE_AESECB_ENC(&p_ctx->aesBlkCipher, &p_ctx->aesKeyRef))
    {
        result = MBA_RES_FAIL;
    }
    else
    {
        result = MBA_RES_SUCCESS;
    }
    
    SX_CLK_DISABLE();

    return result;

}

uint16_t MW_AES_AesEcbEncrypt(MW_AES_Ctx_T * p_ctx, uint16_t length, uint8_t *p_chiperText, uint8_t *p_plainText)
{
    int32_t s;

    SX_CLK_ENABLE();

    s = SX_BLKCIPHER_CRYPT(&p_ctx->aesBlkCipher, (char *)p_plainText, length, (char *)p_chiperText);
    if (s == SX_OK)
    {
        s = SX_BLKCIPHER_RUN(&p_ctx->aesBlkCipher);
    }
    if (s == SX_OK)
    {
        s = SX_BLKCIPHER_WAIT(&p_ctx->aesBlkCipher);
    }

    SX_CLK_DISABLE();


    if (s != SX_OK)
    {
        return MBA_RES_FAIL;
    }

    return MBA_RES_SUCCESS;
}

