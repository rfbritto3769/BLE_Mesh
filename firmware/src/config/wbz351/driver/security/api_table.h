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


#include <stdint.h>


#ifndef _API_TABLE_H
#define _API_TABLE_H

extern const void * const api_table[];

/* Enable Silex/BA457 Clock */
#define SX_CLK_ENABLE()                  SILEX_REGS->SILEX_CRYPTOCON = SILEX_CRYPTOCON_ENABLE(1)

/* Disable Silex/BA457 Clock */
#define SX_CLK_DISABLE()                 SILEX_REGS->SILEX_CRYPTOCON = SILEX_CRYPTOCON_ENABLE(0)

#define API_TABLE_BASE_ADDRESS                  (uint32_t)0xF000

#define ATO_AEAD                                        0
#define ATO_SX_AEAD_CREATE_AESGCM_ENC                   (ATO_AEAD + 0)
#define ATO_SX_AEAD_CREATE_AESGCM_DEC                   (ATO_AEAD + 4)
#define ATO_SX_AEAD_CREATE_AESCCM_ENC                   (ATO_AEAD + 8)
#define ATO_SX_AEAD_CREATE_AESCCM_DEC                   (ATO_AEAD + 12)
#define ATO_SX_AEAD_FEED_AAD                            (ATO_AEAD + 16)
#define ATO_SX_AEAD_CRYPT                               (ATO_AEAD + 20)
#define ATO_SX_AEAD_PRODUCE_TAG                         (ATO_AEAD + 24)
#define ATO_SX_AEAD_VERIFY_TAG                          (ATO_AEAD + 28)
#define ATO_SX_AEAD_RESUME_STATE                        (ATO_AEAD + 32)
#define ATO_SX_AEAD_SAVE_STATE                          (ATO_AEAD + 36)
#define ATO_SX_AEAD_WAIT                                (ATO_AEAD + 40)
#define ATO_SX_AEAD_STATUS                              (ATO_AEAD + 44)

#define ATO_BLOCK_CIPHER                                (ATO_SX_AEAD_STATUS + 4) 
#define ATO_SX_BLKCIPHER_CREATE_AESXTS_ENC              (ATO_BLOCK_CIPHER + 0)  //48
#define ATO_SX_BLKCIPHER_CREATE_AESXTS_DEC              (ATO_BLOCK_CIPHER + 4)
#define ATO_SX_BLKCIPHER_CREATE_AESCTR_ENC              (ATO_BLOCK_CIPHER + 8)
#define ATO_SX_BLKCIPHER_CREATE_AESCTR_DEC              (ATO_BLOCK_CIPHER + 12)
#define ATO_SX_BLKCIPHER_CREATE_AESECB_ENC              (ATO_BLOCK_CIPHER + 16)
#define ATO_SX_BLKCIPHER_CREATE_AESECB_DEC              (ATO_BLOCK_CIPHER + 20)
#define ATO_SX_BLKCIPHER_CREATE_AESCBC_ENC              (ATO_BLOCK_CIPHER + 24)
#define ATO_SX_BLKCIPHER_CREATE_AESCBC_DEC              (ATO_BLOCK_CIPHER + 28)
#define ATO_SX_BLKCIPHER_CREATE_AESCFB_ENC              (ATO_BLOCK_CIPHER + 32)
#define ATO_SX_BLKCIPHER_CREATE_AESCFB_DEC              (ATO_BLOCK_CIPHER + 36)
#define ATO_SX_BLKCIPHER_CREATE_AESOFB_ENC              (ATO_BLOCK_CIPHER + 40)
#define ATO_SX_BLKCIPHER_CREATE_AESOFB_DEC              (ATO_BLOCK_CIPHER + 44)
#define ATO_SX_BLKCIPHER_CRYPT                          (ATO_BLOCK_CIPHER + 48)
#define ATO_SX_BLKCIPHER_RUN                            (ATO_BLOCK_CIPHER + 52)
#define ATO_SX_BLKCIPHER_RESUME_STATE                   (ATO_BLOCK_CIPHER + 56)
#define ATO_SX_BLKCIPHER_SAVE_STATE                     (ATO_BLOCK_CIPHER + 60)
#define ATO_SX_BLKCIPHER_WAIT                           (ATO_BLOCK_CIPHER + 64)
#define ATO_SX_BLKCIPHER_STATUS                         (ATO_BLOCK_CIPHER + 68)

#define ATO_CMMASK                                      (ATO_SX_BLKCIPHER_STATUS + 4) 
#define ATO_SX_CM_LOAD_MASK                             (ATO_CMMASK + 0) //120
#define ATO_SX_CM_LOAD_MASK_WAIT                        (ATO_CMMASK + 4)
#define ATO_SX_CM_LOAD_MASK_STATUS                      (ATO_CMMASK + 8)

#define ATO_HASH                                        (ATO_SX_CM_LOAD_MASK_STATUS + 4)
#define ATO_SX_HASH_CREATE                              (ATO_HASH + 0) //132
#define ATO_SX_HASH_GET_ALG_DIGESTSZ                    (ATO_HASH + 4)
#define ATO_SX_HASH_GET_ALG_BLOCKSZ                     (ATO_HASH + 8)
#define ATO_SX_HASH_FEED                                (ATO_HASH + 12)
#define ATO_SX_HASH_RESUME_STATE                        (ATO_HASH + 16)
#define ATO_SX_HASH_SAVE_STATE                          (ATO_HASH + 20)
#define ATO_SX_HASH_DIGEST                              (ATO_HASH + 24)
#define ATO_SX_HASH_WAIT                                (ATO_HASH + 28)
#define ATO_SX_HASH_STATUS                              (ATO_HASH + 32)
#define ATO_SX_HASH_GET_DIGESTSZ                        (ATO_HASH + 36)
#define ATO_SX_HASH_GET_BLOCKSZ                         (ATO_HASH + 40)
#define ATO_SX_HASH_ABANDON                             (ATO_HASH + 44)

#define ATO_SHA_1                                       (ATO_SX_HASH_ABANDON + 4)
#define ATO_SX_HASH_CREATE_SHA1                         (ATO_SHA_1 + 0) // 180

#define ATO_SHA_2                                       (ATO_SX_HASH_CREATE_SHA1 + 4)
#define ATO_SX_HASH_CREATE_SHA224                       (ATO_SHA_2 + 0) //184
#define ATO_SX_HASH_CREATE_SHA256                       (ATO_SHA_2 + 4)
#define ATO_SX_HASH_CREATE_SHA384                       (ATO_SHA_2 + 8)
#define ATO_SX_HASH_CREATE_SHA512                       (ATO_SHA_2 + 12)

#define ATO_INTERRUPTS                                  (ATO_SX_HASH_CREATE_SHA512 + 4)
#define ATO_SX_INTERRUPTS_ENABLE                        (ATO_INTERRUPTS + 0) //200
#define ATO_SX_INTERRUPTS_DISABLE                       (ATO_INTERRUPTS + 4)

#define ATO_KEYREF                                      (ATO_SX_INTERRUPTS_DISABLE + 4)
#define ATO_SX_KEYREF_LOAD_MATERIAL                     (ATO_KEYREF + 0) //208
#define ATO_SX_KEYREF_LOAD_BY_ID                        (ATO_KEYREF + 4)

#define ATO_MAC                                         (ATO_SX_KEYREF_LOAD_BY_ID + 4)
#define ATO_SX_MAC_FEED                                 (ATO_MAC + 0) //216
#define ATO_SX_MAC_GENERATE                             (ATO_MAC + 4)
#define ATO_SX_MAC_RESUME_STATE                         (ATO_MAC + 8)
#define ATO_SX_MAC_SAVE_STATE                           (ATO_MAC + 12)
#define ATO_SX_MAC_WAIT                                 (ATO_MAC + 16)
#define ATO_SX_MAC_STATUS                               (ATO_MAC + 20)

#define ATO_CMAC                                        (ATO_SX_MAC_STATUS + 4)
#define ATO_SX_MAC_CREATE_AESCMAC                       (ATO_CMAC + 0) //240

#define ATO_HMAC                                        (ATO_SX_MAC_CREATE_AESCMAC + 4)
#define ATO_SX_MAC_CREATE_HMAC_SHA1                     (ATO_HMAC + 0) //244
#define ATO_SX_MAC_CREATE_HMAC_SHA2_224                   (ATO_HMAC + 4)
#define ATO_SX_MAC_CREATE_HMAC_SHA2_256                   (ATO_HMAC + 8)
#define ATO_SX_MAC_CREATE_HMAC_SHA2_384                   (ATO_HMAC + 12)
#define ATO_SX_MAC_CREATE_HMAC_SHA2_512                   (ATO_HMAC + 16)

#define ATO_TRNG                                        (ATO_SX_MAC_CREATE_HMAC_SHA2_512 + 4)
#define ATO_SX_TRNG_INIT                                (ATO_TRNG + 0) //264
#define ATO_SX_TRNG_GET                                 (ATO_TRNG + 4)

#define ATO_PK_CORE                                     (ATO_SX_TRNG_GET + 4)
#define ATO_SX_PK_LIST_CONSTRAINTS                      (ATO_PK_CORE + 0) //272
#define ATO_SX_PK_FETCH_CAPABILITIES                    (ATO_PK_CORE + 4)
#define ATO_SX_PK_OPEN                                  (ATO_PK_CORE + 8)
#define ATO_SX_PK_CLOSE                                 (ATO_PK_CORE + 12)
#define ATO_SX_PK_ACQUIRE_REQ                           (ATO_PK_CORE + 16)
#define ATO_SX_PK_GET_REQ_ID                            (ATO_PK_CORE + 20)
#define ATO_SX_PK_SET_USER_CONTEXT                      (ATO_PK_CORE + 24)
#define ATO_SX_PK_GET_USER_CONTEXT                      (ATO_PK_CORE + 28)
#define ATO_SX_PK_GET_OPSIZE                            (ATO_PK_CORE + 32)
#define ATO_SX_PK_LIST_ECC_INSLOTS                      (ATO_PK_CORE + 36)
#define ATO_SX_PK_LIST_GFP_INSLOTS                      (ATO_PK_CORE + 40)
#define ATO_SX_PK_RUN                                   (ATO_PK_CORE + 44)
#define ATO_SX_PK_GET_STATUS                            (ATO_PK_CORE + 48)
#define ATO_SX_PK_WAIT                                  (ATO_PK_CORE + 52)
//#define ATO_SX_PK_CLEARIRQ                              (ATO_PK_CORE + 56)
#define ATO_SX_PK_POP_FINISHED_REQ                      (ATO_PK_CORE + 56)
#define ATO_SX_PK_GET_GLOBAL_NOTIFICATION_ID            (ATO_PK_CORE + 60)
#define ATO_SX_PK_GET_REQ_COMPLETION_ID                 (ATO_PK_CORE + 64)
#define ATO_SX_PK_GET_OUTPUT_OPS                        (ATO_PK_CORE + 68)
#define ATO_SX_PK_RELEASE_REQ                           (ATO_PK_CORE + 72)

#define ATO_ECC_CURVES                                  (ATO_SX_PK_RELEASE_REQ + 4)
#define ATO_SX_PK_CREATE_ECP_CURVE                      (ATO_ECC_CURVES + 0) //348
#define ATO_SX_PK_CREATE_ECB_CURVE                      (ATO_ECC_CURVES + 4)
#define ATO_SX_PK_DESTROY_EC_CURVE                      (ATO_ECC_CURVES + 8)
#define ATO_SX_PK_GET_CURVE_NISTP192                    (ATO_ECC_CURVES + 12)
#define ATO_SX_PK_GET_CURVE_NISTP256                    (ATO_ECC_CURVES + 16)
#define ATO_SX_PK_GET_CURVE_NISTP384                    (ATO_ECC_CURVES + 20)
#define ATO_SX_PK_GET_CURVE_NISTP521                    (ATO_ECC_CURVES + 24)
#define ATO_SX_PK_GET_CURVE_ED25519                     (ATO_ECC_CURVES + 28)
#define ATO_SX_PK_GET_CURVE_ED448                       (ATO_ECC_CURVES + 32)
#define ATO_SX_PK_GET_CURVE_X25519                      (ATO_ECC_CURVES + 36)
#define ATO_SX_PK_GET_CURVE_X448                        (ATO_ECC_CURVES + 40)
#define ATO_SX_PK_GET_CURVE_SECP256K1                   (ATO_ECC_CURVES + 44)
#define ATO_SX_PK_GET_CURVE_FP256                       (ATO_ECC_CURVES + 48)
#define ATO_SX_PK_WRITE_CURVE_GEN                       (ATO_ECC_CURVES + 52)
#define ATO_SX_PK_CURVE_OPSIZE                          (ATO_ECC_CURVES + 56)

#define ATO_ED25519                                     (ATO_SX_PK_CURVE_OPSIZE + 4)
#define ATO_SX_ED25519_PTMULT                           (ATO_ED25519 + 0) //408
#define ATO_SX_ASYNC_ED25519_PTMULT_GO                  (ATO_ED25519 + 4)
#define ATO_SX_ASYNC_ED25519_PTMULT_END                 (ATO_ED25519 + 8)
#define ATO_SX_ED25519_SIGN                             (ATO_ED25519 + 12)
#define ATO_SX_PK_ASYNC_ED25519_SIGN_GO                 (ATO_ED25519 + 16)
#define ATO_SX_ASYNC_ED25519_SIGN_END                   (ATO_ED25519 + 20)
#define ATO_SX_ED25519_VERIFY                           (ATO_ED25519 + 24)
#define ATO_SX_ASYNC_ED25519_VERIFY_GO                  (ATO_ED25519 + 28)

#define ATO_ED448                                       (ATO_SX_ASYNC_ED25519_VERIFY_GO + 4)
#define ATO_SX_ED448_PTMULT                             (ATO_ED448 + 0) //440
#define ATO_SX_ASYNC_ED448_PTMULT_GO                    (ATO_ED448 + 4)
#define ATO_SX_ASYNC_ED448_PTMULT_END                   (ATO_ED448 + 8)
#define ATO_SX_ED448_SIGN                               (ATO_ED448 + 12)
#define ATO_SX_PK_ASYNC_ED448_SIGN_GO                   (ATO_ED448 + 16)
#define ATO_SX_ASYNC_ED448_SIGN_END                     (ATO_ED448 + 20)
#define ATO_SX_ED448_VERIFY                             (ATO_ED448 + 24)
#define ATO_SX_ASYNC_ED448_VERIFY_GO                    (ATO_ED448 + 28)

#define ATO_MONTGOMERY                                  (ATO_SX_ASYNC_ED448_VERIFY_GO + 4)
#define ATO_SX_X25519_PTMULT                            (ATO_MONTGOMERY + 0) //472
#define ATO_SX_ASYNC_X25519_PTMULT_GO                   (ATO_MONTGOMERY + 4)
#define ATO_SX_ASYNC_X25519_PTMULT_END                  (ATO_MONTGOMERY + 8)
#define ATO_SX_X448_PTMULT                              (ATO_MONTGOMERY + 12)
#define ATO_SX_ASYNC_X448_PTMULT_GO                     (ATO_MONTGOMERY + 16)
#define ATO_SX_ASYNC_X448_PTMULT_END                    (ATO_MONTGOMERY + 20)

#define ATO_ADAPTER                                     (ATO_SX_ASYNC_X448_PTMULT_END + 4)
#define ATO_SX_PK_OP2MEM_LE                             (ATO_ADAPTER + 0) //496
#define ATO_SX_PK_OP2MEM_BE                             (ATO_ADAPTER + 4)
#define ATO_SX_PK_OP2MEM                                (ATO_ADAPTER + 8)
#define ATO_SX_PK_OP2VMEM_LE                            (ATO_ADAPTER + 12)
#define ATO_SX_PK_OP2VMEM_BE                            (ATO_ADAPTER + 16)
#define ATO_SX_PK_OP2VMEM                               (ATO_ADAPTER + 20)
#define ATO_SX_PK_MEM2OP_LE                             (ATO_ADAPTER + 24)
#define ATO_SX_PK_MEM2OP_BE                             (ATO_ADAPTER + 28)
#define ATO_SX_PK_MEM2OP                                (ATO_ADAPTER + 32)
#define ATO_SX_OP_SIZE                                  (ATO_ADAPTER + 36)

#define ATO_PKSTATUS                                    (ATO_SX_OP_SIZE + 4)
#define ATO_SX_DESCRIBE_STATUSCODE                      (ATO_PKSTATUS + 0) //536

#define ATO_HASH_STRUCTS                                (ATO_SX_DESCRIBE_STATUSCODE + 4)
#define ATO_SXHASHALG_SHA1                              (ATO_HASH_STRUCTS + 0) // 540
#define ATO_SXHASHALG_SHA2_224                          (ATO_HASH_STRUCTS + 4)
#define ATO_SXHASHALG_SHA2_256                          (ATO_HASH_STRUCTS + 8)
#define ATO_SXHASHALG_SHA2_384                          (ATO_HASH_STRUCTS + 12)
#define ATO_SXHASHALG_SHA2_512                          (ATO_HASH_STRUCTS + 16)

#define ATO_PK_COMMANDS                                 (ATO_SXHASHALG_SHA2_512 + 4)
#define ATO_SX_PK_CMD_MOD_ADD                           (ATO_PK_COMMANDS + 0) //560
#define ATO_SX_PK_CMD_MOD_SUB                           (ATO_PK_COMMANDS + 4)
#define ATO_SX_PK_CMD_ODD_MOD_MULT                      (ATO_PK_COMMANDS + 8)
#define ATO_SX_PK_CMD_EVEN_MOD_INV                      (ATO_PK_COMMANDS + 12)
#define ATO_SX_PK_CMD_EVEN_MOD_REDUCE                   (ATO_PK_COMMANDS + 16)
#define ATO_SX_PK_CMD_ODD_MOD_REDUCE                    (ATO_PK_COMMANDS + 20)
#define ATO_SX_PK_CMD_ODD_MOD_DIV                       (ATO_PK_COMMANDS + 24)
#define ATO_SX_PK_CMD_ODD_MOD_INV                       (ATO_PK_COMMANDS + 28)
#define ATO_SX_PK_CMD_MOD_SQRT                          (ATO_PK_COMMANDS + 32)
#define ATO_SX_PK_CMD_MULT                              (ATO_PK_COMMANDS + 36)
#define ATO_SX_PK_CMD_MOD_EXP                           (ATO_PK_COMMANDS + 40)
#define ATO_SX_PK_CMD_DH_MOD_EXP_CM                     (ATO_PK_COMMANDS + 44)
#define ATO_SX_PK_CMD_RSA_MOD_EXP_CM                    (ATO_PK_COMMANDS + 48)
#define ATO_SX_PK_CMD_MOD_EXP_CRT                       (ATO_PK_COMMANDS + 52)
#define ATO_SX_PK_CMD_MOD_EXP_CRT_CM                    (ATO_PK_COMMANDS + 56)
#define ATO_SX_PK_CMD_RSA_KEYGEN                        (ATO_PK_COMMANDS + 60)
#define ATO_SX_PK_CMD_RSA_CRT_KEYPARAMS                 (ATO_PK_COMMANDS + 64)
#define ATO_SX_PK_CMD_MONTGOMERY_PTMUL                  (ATO_PK_COMMANDS + 68)
#define ATO_SX_PK_CMD_ECDSA_VER                         (ATO_PK_COMMANDS + 72)
#define ATO_SX_PK_CMD_ECDSA_GEN                         (ATO_PK_COMMANDS + 76)
#define ATO_SX_PK_CMD_ECC_PT_ADD                        (ATO_PK_COMMANDS + 80)
#define ATO_SX_PK_CMD_ECC_PTMUL                         (ATO_PK_COMMANDS + 84)
#define ATO_SX_PK_CMD_ECC_PTMUL_CM                      (ATO_PK_COMMANDS + 88)
#define ATO_SX_PK_CMD_ECC_PT_DECOMP                     (ATO_PK_COMMANDS + 92)
#define ATO_SX_PK_CMD_CHECK_PARAM_AB                    (ATO_PK_COMMANDS + 96)
#define ATO_SX_PK_CMD_CHECK_PARAM_N                     (ATO_PK_COMMANDS + 100)
#define ATO_SX_PK_CMD_CHECK_XY                          (ATO_PK_COMMANDS + 104)
#define ATO_SX_PK_CMD_ECC_PT_DOUBLE                     (ATO_PK_COMMANDS + 108)
#define ATO_SX_PK_CMD_ECC_PTONCURVE                     (ATO_PK_COMMANDS + 112)
#define ATO_SX_PK_CMD_EDDSA_PTMUL                       (ATO_PK_COMMANDS + 116)
#define ATO_SX_PK_CMD_EDDSA_SIGN                        (ATO_PK_COMMANDS + 120)
#define ATO_SX_PK_CMD_EDDSA_VER                         (ATO_PK_COMMANDS + 124)
#define ATO_SX_PK_CMD_DSA_SIGN                          (ATO_PK_COMMANDS + 128)
#define ATO_SX_PK_CMD_DSA_VER                           (ATO_PK_COMMANDS + 132)

#define ATO_IMG_MEM                                     (ATO_SX_PK_CMD_DSA_VER + 4)
#define ATO_IMG_MEM_FindValidTopologies                 (ATO_IMG_MEM + 0) // 696
#define ATO_IMG_MEM_CacheHeader                         (ATO_IMG_MEM + 4)
#define ATO_IMG_MEM_ValidateHeader                      (ATO_IMG_MEM + 8)
#define ATO_IMG_MEM_CacheAndValidateHeaders             (ATO_IMG_MEM + 12)
#define ATO_IMG_MEM_GetTopologyByAddress                (ATO_IMG_MEM + 16)
#define ATO_IMG_MEM_SlotSort                            (ATO_IMG_MEM + 20) // 720 -  136 API's + 43 structures   

#endif

