/** Asymmetric cryptographic command definitions
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2014-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef CMDDEFS_HEADER_FILE
#define CMDDEFS_HEADER_FILE

#include <stdint.h>
#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sx_pk_cmd_def;

typedef const struct sx_pk_cmd_def * const SX_CMD_PTR;

/**
 * @addtogroup SX_PK_CMDS
 *
 * @{
 */

/** Modular addition of operands A and B */
#define SX_PK_CMD_MOD_ADD                  ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_ADD))

/** Modular substraction of operands A and B */
#define SX_PK_CMD_MOD_SUB                  ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_SUB))

/** Modular multiplication of operands A and B with odd modulo */
#define SX_PK_CMD_ODD_MOD_MULT             ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ODD_MOD_MULT))

/** Modular inversion of an operand with even modulo */
#define SX_PK_CMD_EVEN_MOD_INV             ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_EVEN_MOD_INV))

/** Modular inversion of an operand with even modulo */
#define SX_PK_CMD_EVEN_MOD_REDUCE          ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_EVEN_MOD_REDUCE))

/** Modular reduction of an operand with odd modulo */
#define SX_PK_CMD_ODD_MOD_REDUCE           ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ODD_MOD_REDUCE))

/** Modular division of operands A and B with odd modulo */
#define SX_PK_CMD_ODD_MOD_DIV              ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ODD_MOD_DIV))

/** Modular inversion of an operand with odd modulo */
#define SX_PK_CMD_ODD_MOD_INV              ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ODD_MOD_INV))

/** Modular square root **/
#define SX_PK_CMD_MOD_SQRT                 ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_SQRT))

/** Multiplication **/
#define SX_PK_CMD_MULT                     ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MULT))

/** Modular exponentiation operation */
#define SX_PK_CMD_MOD_EXP                  ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_EXP))

/** Diffie Hellman modular exponentiation operation with countermeasures
 *
 * The modulus must be a prime number.
 */
#define SX_PK_CMD_DH_MOD_EXP_CM            ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_DH_MOD_EXP_CM))

/** RSA modular exponentiation operation with countermeasures*/
#define SX_PK_CMD_RSA_MOD_EXP_CM           ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_RSA_MOD_EXP_CM))

/** Modular exponentiation operation (for RSA) with Chinese Remainder Theorem */
#define SX_PK_CMD_MOD_EXP_CRT              ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_EXP_CRT))

/** Modular exponentiation operation (for RSA) with Chinese Remainder Theorem
 *
 * With blinding factor for countermeasures.
 */
#define SX_PK_CMD_MOD_EXP_CRT_CM           ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MOD_EXP_CRT_CM))

/** RSA private key including lambda(n) computation from primes
 *
 * Lambda(n) is also called the Carmichael's totient function or
 * Carmichael function.
 */
#define SX_PK_CMD_RSA_KEYGEN               ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_RSA_KEYGEN))

/** RSA CRT private key parameters computation */
#define SX_PK_CMD_RSA_CRT_KEYPARAMS        ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_RSA_CRT_KEYPARAMS))

/** Montgomery point multiplication for X25519 and X448
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for X25519 and X448.
 */
#define SX_PK_CMD_MONTGOMERY_PTMUL         ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_MONTGOMERY_PTMUL))

/** Elliptic curve ECDSA signature verification operation */
#define SX_PK_CMD_ECDSA_VER                ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECDSA_VER))

/** Elliptic curve ECDSA signature generation operation */
#define SX_PK_CMD_ECDSA_GEN                ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECDSA_GEN))

/** Elliptic curve point addition operation */
#define SX_PK_CMD_ECC_PT_ADD               ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PT_ADD))

/** Elliptic curve point multiplication operation */
#define SX_PK_CMD_ECC_PTMUL                ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PTMUL))

/** Elliptic curve point multiplication operation with countermeasures */
#define SX_PK_CMD_ECC_PTMUL_CM             ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PTMUL_CM))

/** Elliptic curve point decompression operation */
#define SX_PK_CMD_ECC_PT_DECOMP            ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PT_DECOMP))

/** Elliptic curve check parameters a & b */
#define SX_PK_CMD_CHECK_PARAM_AB           ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_CHECK_PARAM_AB))

/** Elliptic curve check parameter n != p */
#define SX_PK_CMD_CHECK_PARAM_N            ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_CHECK_PARAM_N))

/** Elliptic curve check x,y point < p */
#define SX_PK_CMD_CHECK_XY                 ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_CHECK_XY))

/** Elliptic curve point doubling */
#define SX_PK_CMD_ECC_PT_DOUBLE            ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PT_DOUBLE))

/** Elliptic curve point on curve check */
#define SX_PK_CMD_ECC_PTONCURVE            ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_ECC_PTONCURVE))

/** EDDSA point multiplication operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for ED25519.
 */
#define SX_PK_CMD_EDDSA_PTMUL              ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_EDDSA_PTMUL))

/** EDDSA 2nd part of signature operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for ED25519.
 */
#define SX_PK_CMD_EDDSA_SIGN               ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_EDDSA_SIGN))

/** EDDSA signature verification operation
 *
 * All operands for this command use a little endian representation.
 * Operands should be decoded and clamped as defined in specifications
 * for ED25519.
 */
#define SX_PK_CMD_EDDSA_VER                ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_EDDSA_VER))

/** DSA signature generation */
#define SX_PK_CMD_DSA_SIGN                 ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_DSA_SIGN))

/** DSA signature verification */
#define SX_PK_CMD_DSA_VER                  ((SX_CMD_PTR)**(uint32_t **)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CMD_DSA_VER))

/** @} */

#ifdef __cplusplus
}
#endif

#endif
