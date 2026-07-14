/** Predefined and custom elliptic curve definitions
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef EC_CURVES_HEADER_FILE
#define EC_CURVES_HEADER_FILE

#include <stdint.h>
#include <driver/security/silexpk/core_api.h>
#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup SX_PK_CURVES
 *
 * @{
 */

/** Slots to write custom curve parameters into. */
struct sx_curve_slots {
    struct sx_pk_slot p; /**< Field size of curve **/
    struct sx_pk_slot n; /**< Order of curve **/
    struct sx_pk_slot gx; /**< x-coordinate of generator point of curve **/
    struct sx_pk_slot gy; /**< y-coordinate of generator point of curve **/
    struct sx_pk_slot a; /**< Curve parameter a **/
    struct sx_pk_slot b; /**< Curve parameter b **/
};

/** Create a prime elliptic curve
 *
 * @remark When this function returns, copy the paramters of the
 * curve into the slots returned in 'slots'. After that, you can
 * use the prime elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] curve Curve to initialise as a prime elliptic curve
 * @param[in] mem Memory for the parameters of the curve. The size of
 * the memory should be 6 time sz as there are 6 curve parameters of size sz
 * @param[in] sz Size of the curve in bytes
 * @param[out] slots Slots for the curve parameters
 *
 * @see SX_PK_CREATE_ECB_CURVE() and SX_PK_DESTROY_EC_CURVE()
 */
typedef void (*FUNC_SX_PK_CREATE_ECP_CURVE)( struct sx_pk_cnx *cnx, struct sx_pk_ecurve *curve, char *mem, int sz, struct sx_curve_slots *slots);
#define SX_PK_CREATE_ECP_CURVE         ((FUNC_SX_PK_CREATE_ECP_CURVE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CREATE_ECP_CURVE)))

/** Create a binary elliptic curve
 *
 * @remark When this function returns, copy the paramters of the
 * curve into the slots returned in 'slots'. After that, you can
 * use the binary elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] curve Curve to initialise as a binary elliptic curve
 * @param[in] mem Memory for the parameters of the curve. The size of
 * the memory should be 6 time sz as there are 6 curve parameters of size sz
 * @param[in] sz Size of the curve in bytes
 * @param[out] slots Slots for the curve parameters
 *
 * @see SX_PK_CREATE_ECP_CURVE() and SX_PK_DESTROY_EC_CURVE()
 */
typedef void (*FUNC_SX_PK_CREATE_ECB_CURVE)(struct sx_pk_cnx *cnx, struct sx_pk_ecurve *curve, char *mem, int sz, struct sx_curve_slots *slots);
#define SX_PK_CREATE_ECB_CURVE         ((FUNC_SX_PK_CREATE_ECB_CURVE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CREATE_ECB_CURVE)))

/** Destroy a custom ecp or ecb curve
 *
 * Destroy a curve created previously by SX_PK_CREATE_ECP_CURVE()
 * or SX_PK_CREATE_ECB_CURVE().
 *
 * @param[in] curve Initialised curve to destroy
 */
typedef void (*FUNC_SX_PK_DESTROY_EC_CURVE)(struct sx_pk_ecurve *curve);
#define SX_PK_DESTROY_EC_CURVE         ((FUNC_SX_PK_DESTROY_EC_CURVE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_DESTROY_EC_CURVE)))


/** Get a reference to the predefined NIST P192 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for P192 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_NISTP192)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_NISTP192         ((FUNC_SX_PK_GET_CURVE_NISTP192)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_NISTP192)))

/** Get a reference to the predefined NIST P256 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for P256 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_NISTP256)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_NISTP256         ((FUNC_SX_PK_GET_CURVE_NISTP256)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_NISTP256)))

/** Get a reference to the predefined NIST P384 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for P384 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_NISTP384)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_NISTP384         ((FUNC_SX_PK_GET_CURVE_NISTP384)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_NISTP384)))

/** Get a reference to the predefined NIST P521 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for P521 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_NISTP521)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_NISTP521         ((FUNC_SX_PK_GET_CURVE_NISTP521)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_NISTP521)))

/** Get a reference to the predefined ED25519 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for ED25519 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_ED25519)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_ED25519         ((FUNC_SX_PK_GET_CURVE_ED25519)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_ED25519)))

/** Get a reference to the predefined ED448 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for ED448 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_ED448)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_ED448         ((FUNC_SX_PK_GET_CURVE_ED448)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_ED448)))

/** Get a reference to the predefined X25519 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for X25519 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_X25519)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_X25519         ((FUNC_SX_PK_GET_CURVE_X25519)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_X25519)))

/** Get a reference to the predefined X448 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for X448 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_X448)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_X448         ((FUNC_SX_PK_GET_CURVE_X448)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_X448)))

/** Get a reference to the predefined SEC p256k1 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for SEC p256k1 curve
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_SECP256K1)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_SECP256K1         ((FUNC_SX_PK_GET_CURVE_SECP256K1)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_SECP256K1)))

/** Get a reference to the predefined fp256 elliptic curve
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @return Curve structure for fp256 curve (used in SM2)
 */
typedef struct sx_pk_ecurve (*FUNC_SX_PK_GET_CURVE_FP256)(struct sx_pk_cnx *cnx);
#define SX_PK_GET_CURVE_FP256         ((FUNC_SX_PK_GET_CURVE_FP256)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_GET_CURVE_FP256)))

/** Write the generator point of the curve into the slots (internal)
 *
 * Write the parameter gx & gy from curve to px.addr & py.addr respectively
 *
 * @param[in] pk The accelerator request
 * @param[in] curve Initialised curve to get generator point from.
 * @param[in] px x-coordinate slot of generator point. The curve generator
 * (x-coordinate) will be written to this address
 * @param[in] py y-coordinate slot of generator point. The curve generator
 * (y-coordinate) will be written to this address
 */
typedef void (*FUNC_SX_PK_WRITE_CURVE_GEN)(sx_pk_accel *pk, const struct sx_pk_ecurve *curve, struct sx_pk_slot px, struct sx_pk_slot py);
#define SX_PK_WRITE_CURVE_GEN         ((FUNC_SX_PK_WRITE_CURVE_GEN)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CREATE_AESGCM_ENC)))

/** Return the operand size in bytes for the given curve
 *
 * @param[in] curve Initialised curve to get operand size from
 * @return Operand size in bytes for the given curve
*/
typedef int (*FUNC_SX_PK_CURVE_OPSIZE)(const struct sx_pk_ecurve *curve);
#define SX_PK_CURVE_OPSIZE         ((FUNC_SX_PK_CURVE_OPSIZE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_CURVE_OPSIZE)))

/** @} */

#ifdef __cplusplus
}
#endif

#endif
