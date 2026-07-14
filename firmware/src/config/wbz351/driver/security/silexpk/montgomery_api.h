/** Simpler functions for base Montgomery elliptic curve operations
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef MONTGOMERY_HEADER_FILE
#define MONTGOMERY_HEADER_FILE

#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Size in bytes of a point on X25519 curve */
#define SX_X25519_PT_SZ 32

/** Size in bytes of a point on X448 curve */
#define SX_X448_PT_SZ 56

/**
 * @addtogroup SX_PK_MONT
 *
 * @{
 */

/** An X25519 point */
struct sx_x25519_pt {
    /** Bytes array representation of a X25519 point **/
    char bytes[SX_X25519_PT_SZ];
};

/** An X448 point */
struct sx_x448_pt {
    /** Bytes array representation of a X448 point  **/
    char bytes[SX_X448_PT_SZ];
};


/** Montgomery point multiplication (X25519)
 *
 * Compute r = pt * k
 *
 * The operands must be decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Scalar
 * @param[in] pt Point on the X25519 curve
 * @param[out] r Multiplication result of k and pt
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see SX_ASYNC_X25519_PTMULT_GO() and SX_ASYNC_X25519_PTMULT_END()
 * for an asynchronous version
 */
typedef int (*FUNC_SX_X25519_PTMULT)(struct sx_pk_cnx *cnx, const struct sx_x25519_pt *k, const struct sx_x25519_pt *pt, struct sx_x25519_pt *r);
#define SX_X25519_PTMULT         ((FUNC_SX_X25519_PTMULT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_X25519_PTMULT)))

/** Asynchronous Montgomery point multiplication (X25519)
 *
 * Start a montgomery point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_ASYNC_X25519_PTMULT_END()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Scalar
 * @param[in] pt Point on the X25519 curve
 *
 * @return Acquired acceleration request for this operation
 *
 * @see SX_ASYNC_X25519_PTMULT_END() and SX_X25519_PTMULT()
 */
typedef struct sx_pk_dreq (*FUNC_SX_ASYNC_X25519_PTMULT_GO)(struct sx_pk_cnx *cnx, const struct sx_x25519_pt *k, const struct sx_x25519_pt *pt);
#define SX_ASYNC_X25519_PTMULT_GO         ((FUNC_SX_ASYNC_X25519_PTMULT_GO)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_X25519_PTMULT_GO)))

/** Collect the result of asynchronous Montgomery point multiplication (X25519)
 *
 * Get the output operand of the Montgomery point multiplication
 * and release accelerator.
 *
 * @remark The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Multiplication result of k and pt
 *
 * @see SX_ASYNC_X25519_PTMULT_GO() and sx_async_x25519_ptmult()
 */
typedef void (*FUNC_SX_ASYNC_X25519_PTMULT_END)(sx_pk_accel *req, struct sx_x25519_pt *r);
#define SX_ASYNC_X25519_PTMULT_END         ((FUNC_SX_ASYNC_X25519_PTMULT_END)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_X25519_PTMULT_END)))


/** Montgomery point multiplication (X448)
 *
 * Compute r = pt * k
 *
 * The operands must be decoded and clamped as defined in specifications
 * for X25519 and X448.
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Scalar
 * @param[in] pt Point on the X448 curve
 * @param[out] r Multiplication result of k and pt
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see SX_ASYNC_X448_PTMULT_GO() and SX_ASYNC_X448_PTMULT_END()
 * for an asynchronous version
 */
typedef int (*FUNC_SX_X448_PTMULT)(struct sx_pk_cnx *cnx, const struct sx_x448_pt *k, const struct sx_x448_pt *pt, struct sx_x448_pt *r);
#define SX_X448_PTMULT         ((FUNC_SX_X448_PTMULT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_X448_PTMULT)))

/** Asynchronous Montgomery point multiplication (X448)
 *
 * Start a montgomery point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_ASYNC_X448_PTMULT_END()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Scalar
 * @param[in] pt Point on the X448 curve
 *
 * @return Acquired acceleration request for this operation
 *
 * @see SX_ASYNC_X448_PTMULT_END() and SX_X448_PTMULT()
 */
typedef struct sx_pk_dreq (*FUNC_SX_ASYNC_X448_PTMULT_GO)(struct sx_pk_cnx *cnx, const struct sx_x448_pt *k, const struct sx_x448_pt *pt);
#define SX_ASYNC_X448_PTMULT_GO         ((FUNC_SX_ASYNC_X448_PTMULT_GO)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_X448_PTMULT_GO)))

/** Collect the result of asynchronous Montgomery point multiplication (X448)
 *
 * Get the output operand of the Montgomery point multiplication
 * and release the reserved resources.
 *
 * @remark The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] r Multiplication result of k and pt
 *
 * @see SX_ASYNC_X448_PTMULT_GO() and sx_async_x448_ptmult()
 */
typedef void (*FUNC_SX_ASYNC_X448_PTMULT_END)(sx_pk_accel *req, struct sx_x448_pt *r);
#define SX_ASYNC_X448_PTMULT_END         ((FUNC_SX_ASYNC_X448_PTMULT_END)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_X448_PTMULT_END)))


/**  @} */

#ifdef __cplusplus
}
#endif

#endif
