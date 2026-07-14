/** Simpler functions for base ED448 operations
 *
 * @file
 */
/*
 * Copyright (c) 2020 Silex Insight sa
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ED448_HEADER_FILE
#define ED448_HEADER_FILE

#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Size in bytes of a reduced value in ED448 operations */
#define SX_ED448_SZ  57


/** Size in bytes of an encoded ED448 point */
#define SX_ED448_PT_SZ  57


/** Size in bytes of a digest in ED448 operations */
#define SX_ED448_DGST_SZ  (57 * 2)

/**
 * @addtogroup SX_PK_ED448
 *
 * @{
 */

/** An encoded ED448 point */
struct sx_ed448_pt {
    /** Bytes array representing encoded point for ED448 **/
    char encoded[SX_ED448_PT_SZ];
};

/** A ED448 scalar value */
struct sx_ed448_v {
    /** Bytes array representing scalar for ED448 **/
    char bytes[SX_ED448_SZ];
};


/** A hash digest used in the ED448 protocol */
struct sx_ed448_dgst {
    /** Bytes array of hash digest **/
    char bytes[SX_ED448_DGST_SZ];
};


/** EDDSA point multiplication (ED448)
 *
 * Compute R = r * G, where r is a scalar which can be up to twice the
 * size of the other operands. G is the generator point for the curve.
 * The point R is encoded in pt.
 *
 * When computing the public key, the scalar 'r' is the secret scalar based on
 * the clamped hash digest of the private key.
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] r Secret scalar based on the clamped hash digest of the private key
 * @param[out] pt Encoded resulting R point
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
 * @see SX_ASYNC_ED448_PTMULT_GO() and SX_ASYNC_ED448_PTMULT_END()
 * for an asynchronous version
 */
typedef int (*FUNC_SX_ED448_PTMULT)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *r, struct sx_ed448_pt *pt);
#define SX_ED448_PTMULT         ((FUNC_SX_ED448_PTMULT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ED448_PTMULT)))


/** Asynchronous EDDSA point multiplication (ED448)
 *
 * Start an EDDSA point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_ASYNC_ED448_PTMULT_END()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] r Secret scalar based on the clamped hash digest of the private key
 *
 * @return Acquired acceleration request for this operation
 *
 * @see SX_ASYNC_ED448_PTMULT_END() and SX_ED448_PTMULT()
 */
typedef struct sx_pk_dreq (*FUNC_SX_ASYNC_ED448_PTMULT_GO)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *r);
#define SX_ASYNC_ED448_PTMULT_GO         ((FUNC_SX_ASYNC_ED448_PTMULT_GO)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_ED448_PTMULT_GO)))


/** Collect the result of asynchronous EDDSA point multiplication (ED448)
 *
 * Get the output operands of the EDDSA point multiplication
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] pt Encoded resulting R point
 *
 * @see SX_ASYNC_ED448_PTMULT_GO() and SX_ED448_PTMULT()
 */
typedef void (*FUNC_SX_ASYNC_ED448_PTMULT_END)(sx_pk_accel *req, struct sx_ed448_pt *pt);
#define SX_ASYNC_ED448_PTMULT_END         ((FUNC_SX_ASYNC_ED448_PTMULT_END)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_ED448_PTMULT_END)))


/** Compute signature scalar s for pure EDDSA (ED448).
 *
 * This represents the second step in computing an EDDSA signature.
 *
 * This step computes sig_s :
 *   sig_s = (r + k * s) % l
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] r Secret nonce already used in the first signature step
 * @param[in] s Secret scalar derived from the private key
 * @param[out] sig_s Second part of the EDDSA signature
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
 * @see sx_async_ed448_sign_go() and SX_ASYNC_ED448_SIGN_END()
 * for an asynchronous version
 */
typedef int (*FUNC_SX_ED448_SIGN)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *k, const struct sx_ed448_dgst *r, const struct sx_ed448_v *s, struct sx_ed448_v *sig_s);
#define SX_ED448_SIGN         ((FUNC_SX_ED448_SIGN)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ED448_SIGN)))


/** Asynchronous second part signature generation for pure EDDSA (ED448).
 *
 * Start an ED448 signature generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_ASYNC_ED448_SIGN_END()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] r Secret nonce already used in the first signature step
 * @param[in] s Secret scalar derived from the private key
 *
 * @return Acquired acceleration request for this operation
 *
 * @see SX_ED448_SIGN() and SX_ASYNC_ED448_SIGN_END()
 */
typedef struct sx_pk_dreq  (*FUNC_SX_PK_ASYNC_ED448_SIGN_GO)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *k, const struct sx_ed448_dgst *r, const struct sx_ed448_v *s);
#define SX_PK_ASYNC_ED448_SIGN_GO         ((FUNC_SX_PK_ASYNC_ED448_SIGN_GO)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_PK_ASYNC_ED448_SIGN_GO)))


/** Collect the result of asynchronous computation of ED448 signature scalar
 *
 * Get the output operands of the ED448 signature generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] sig_s Second part of the ED448 signature
 *
 * @see SX_PK_ASYNC_ED448_SIGN_GO() and SX_ED448_SIGN()
 */
typedef void (*FUNC_SX_ASYNC_ED448_SIGN_END)(sx_pk_accel *req, struct sx_ed448_v *sig_s);
#define SX_ASYNC_ED448_SIGN_END         ((FUNC_SX_ASYNC_ED448_SIGN_END)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_ED448_SIGN_END)))


/** Verify an EDDSA signature (ED448)
 *
 * It checks if sig_s * G - k * A matches R.
 *
 * sig_s and the encoded point R form the signature. The points A and R are
 * passed in their encoded form via 'a' and 'r'.
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] a Encoded public key
 * @param[in] sig_s Second part of the signature
 * @param[in] r Encoded first part of the signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see SX_ASYNC_ED448_VERIFY_GO()
 * for an asynchronous version
 */
typedef int (*FUNC_SX_ED448_VERIFY)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *k, const struct sx_ed448_pt *a, const struct sx_ed448_v *sig_s, const struct sx_ed448_pt *r);
#define SX_ED448_VERIFY         ((FUNC_SX_ED448_VERIFY)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ED448_VERIFY)))


/**  Asynchronous (non-blocking) verify an ED448 signature.
 *
 * Start an ED448 signature generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_PK_RELEASE_REQ()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] k Hash of the encoded point R, the public key and the message.
 * It is interpreted as a scalar with a size double of other operands
 * @param[in] a Encoded public key
 * @param[in] sig_s Second part of the signature
 * @param[in] r Encoded first part of the signature
 *
 * @return Acquired acceleration request for this operation
 *
 * @see SX_ED448_VERIFY()
 */
typedef struct sx_pk_dreq (*FUNC_SX_ASYNC_ED448_VERIFY_GO)(struct sx_pk_cnx *cnx, const struct sx_ed448_dgst *k, const struct sx_ed448_pt *a, const struct sx_ed448_v *sig_s, const struct sx_ed448_pt *r);
#define SX_ASYNC_ED448_VERIFY_GO         ((FUNC_SX_ASYNC_ED448_VERIFY_GO)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_ASYNC_ED448_VERIFY_GO)))


/** @} */

#ifdef __cplusplus
}
#endif

#endif
