/** "sxops" interface for Weierstrass elliptic curve computations.
 *
 * Simpler functions to perform public key crypto operations. Included directly
 * in some interfaces (like sxbuf or OpenSSL engine). The functions
 * take input operands (large integers) and output operands
 * which will get the computed results.
 *
 * Operands have the "sx_op" type. The specific interfaces (like sxbuf) define
 * the "sx_op" type.
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ECCWEIERSTRASS_HEADER_FILE
#define ECCWEIERSTRASS_HEADER_FILE

#include <driver/security/silexpk/core_api.h>
#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/security/silexpk/inputslots.h>
#include "driver/security/silexpk/ec_curves_api.h"
#include "driver/security/silexpk/adapter_api.h"
#include "driver/security/silexpk/impl.h"
#include "driver/security/silexpk/cmddefs_api.h"
#include <driver/security/silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_COMPATIBLE(1, 3);

struct sx_pk_ecurve;

/**
 * @addtogroup SX_PK_SXOPS_ECC
 *
 * @{
 */

/** Curve generator point for sx_ecp_ptmult() or sx_async_ecp_mult_go() */
#define SX_PTMULT_CURVE_GENERATOR NULL

/**
 * @addtogroup SX_PK_SXOPS_ECDSA
 *
 * @{
 */

/** Asynchronous ECDSA signature generation
 *
 * Start an ECDSA signature generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecdsa_generate_end()
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA signature
 * @param[in] d Private key
 * @param[in] k Random value
 * @param[in] h Formatted hash digest of message to be signed.
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ecdsa_generate_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *d,
      const sx_op *k,
      const sx_op *h)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecdsa_generate inputs;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECDSA_GEN);
   if (pkreq.status)
      return pkreq;

   // convert and transfer operands
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);
   SX_PK_OP2MEM(d, inputs.d.addr, opsz);
   SX_PK_OP2MEM(k, inputs.k.addr, opsz);
   SX_PK_OP2MEM(h, inputs.h.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** Finish asynchronous (non-blocking) ECDSA generation.
 *
 * Get the output operands of the ECDSA signature generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 */
static inline
void sx_async_ecdsa_generate_end(sx_pk_accel *req, sx_op *r,
      sx_op *s)
{
    sx_async_finish_pair(req, r, s);
}


/** Generate an ECDSA signature on an elliptic curve
 *
 * The signature generation has the following steps :
 *   1. P(x1, y1) = k * G
 *   2. r = x1 mod n
 *   3. if r == 0 then report failure
 *   4. w = k ^ -1 mod n
 *   5. s = k ^ -1 * (h + d * r) mod n
 *   6. if s == 0 then report failure
 *   7. signature is the r and s
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA signature
 * @param[in] d Private key
 * @param[in] k Random value
 * @param[in] h Digest of message to be signed
 * Truncation or padding should be done by user application
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecdsa_generate_go(), sx_async_ecdsa_generate_end() for
 * an asynchronous version
 */
static inline
int sx_ecdsa_generate(
        const struct sx_pk_ecurve *curve,
        const sx_op *d,
        const sx_op *k,
        const sx_op *h,
        sx_op *r,
        sx_op *s)
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ecdsa_generate_go(curve, d, k, h);
   if (pkreq.status)
      return pkreq.status;
   status = SX_PK_WAIT(pkreq.req);
   sx_async_ecdsa_generate_end(pkreq.req, r, s);

   return status;
}


/** Asynchronous (non-blocking) ECDSA verification.
 *
 * Start an ECDSA signature verification on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_PK_RELEASE_REQ()
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA verification
 * @param[in] qx x-coordinate of public key. Point (qx, qy) should be on the curve
 * @param[in] qy y-coordinate of public key. Point (qx, qy) should be on the curve
 * @param[in] r First part of signature to verify
 * @param[in] s Second part of signature to verify
 * @param[in] h Digest of message to be signed
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ecdsa_verify_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *qx,
      const sx_op *qy,
      const sx_op *r,
      const sx_op *s,
      const sx_op *h)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecdsa_verify inputs;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECDSA_VER);
   if (pkreq.status)
      return pkreq;

   // convert and transfer operands
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);
   SX_PK_OP2MEM(qx, inputs.qx.addr, opsz);
   SX_PK_OP2MEM(qy, inputs.qy.addr, opsz);
   SX_PK_OP2MEM(r, inputs.r.addr, opsz);
   SX_PK_OP2MEM(s, inputs.s.addr, opsz);
   SX_PK_OP2MEM(h, inputs.h.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


typedef struct 
{
    struct sx_pk_dreq pkreq;  
    struct sx_pk_config cfg;
} IMAGE_LOADER_CONTEXT_ECDSA;
extern IMAGE_LOADER_CONTEXT_ECDSA iCtxEcdsa;

/** Verify ECDSA signature on an elliptic curve
 *
 *  The verification has the following steps:
 *   1. check qx and qy are smaller than q from the domain
 *   2. Check that Q lies on the elliptic curve from the domain
 *   3. Check that r and s are smaller than n
 *   4.  w = s ^ -1 mod n
 *   5.  u1 = h * w mod n
 *   6.  u2 = r * w mod n
 *   7.  X(x1, y1) = u1 * G + u2 * Q
 *   8. If X is invalid, then the signature is invalid
 *   9.  v = x1 mod n
 *   10. Accept signature if and only if v == r
 *
 *
 * @param[in] curve Elliptic curve on which to perform ECDSA verification
 * @param[in] qx x-coordinate of public key. Point (qx, qy) should be on the curve
 * @param[in] qy y-coordinate of public key. Point (qx, qy) should be on the curve
 * @param[in] r First part of signature to verify
 * @param[in] s Second part of signature to verify
 * @param[in] h Digest of message to be signed
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecdsa_verify_go() for an asynchronous version
 */
static inline
int sx_ecdsa_verify(
        const struct sx_pk_ecurve *curve,
        const sx_op *qx,
        const sx_op *qy,
        const sx_op *r,
        const sx_op *s,
        const sx_op *h)
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ecdsa_verify_go(curve, qx, qy, r, s, h);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   SX_PK_RELEASE_REQ(pkreq.req);

   return status;
}

/** @} */

/**
 * @addtogroup SX_PK_SXOPS_ECOPS
 *
 * @{
 */

/** Asynchronous EC point multiplication.
 *
 * Starts an EC point multiplication on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_mult_end()
 *
 *
 * @param[in] curve Elliptic curve used to perform point multiplication
 * @param[in] k Scalar that multiplies point P
 * @param[in] Py x-coordinate of point P
 * @param[in] Px y-coordinate of point P
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ecp_mult_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *k,
      const sx_op *Px,
      const sx_op *Py)
{
   struct sx_pk_dreq pkreq;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECC_PTMUL);
   if (pkreq.status)
      return pkreq;

   struct sx_pk_inops_ecp_mult inputs;
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);
   SX_PK_OP2MEM(k, inputs.k.addr, opsz);
   if (Px == SX_PTMULT_CURVE_GENERATOR) {
      SX_PK_WRITE_CURVE_GEN(pkreq.req, curve, inputs.px, inputs.py);
   } else {
      SX_PK_OP2MEM(Px, inputs.px.addr, opsz);
      SX_PK_OP2MEM(Py, inputs.py.addr, opsz);
   }

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** Finish asynchronous EC point multiplication.
 *
 * Get the output operands of the EC point multiplication
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] Rx x-coordinate of resulting point R
 * @param[out] Ry y-coordinate of resulting point R
 */
static inline
void sx_async_ecp_mult_end(
      sx_pk_accel *req,
      sx_op *Rx,
      sx_op *Ry)
{
   sx_async_finish_pair(req, Rx, Ry);
}


/** Compute point multiplication on an elliptic curve
 *
 *  (Rx, Ry) = k * (Px, Py)
 *
 *
 * @param[in] curve Elliptic curve used to perform point multiplication
 * @param[in] k Scalar that multiplies point P
 * @param[in] Py x-coordinate of point P
 * @param[in] Px y-coordinate of point P
 * @param[out] Rx x-coordinate of resulting point R
 * @param[out] Ry y-coordinate of resulting point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecp_mult_go(), sx_async_ecp_mult_end()
 * for an asynchronous versions
 */
static inline
int sx_ecp_ptmult(
        const struct sx_pk_ecurve *curve,
        const sx_op *k,
        const sx_op *Px,
        const sx_op *Py,
        sx_op *Rx,
        sx_op *Ry)
{
   int status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ecp_mult_go(curve, k, Px, Py);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   sx_async_ecp_mult_end(pkreq.req, Rx, Ry);

   return status;
}


/** Asynchronous EC point doubling.
 *
 * Starts an EC point doubling on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_double_end()
 *
 *
 * @param[in] curve Elliptic curve used to perform point doubling
 * @param[in] py x-coordinate of point P
 * @param[in] px y-coordinate of point P
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ecp_double_go(
        const struct sx_pk_ecurve *curve,
        const sx_op *px,
        const sx_op *py)
{
   struct sx_pk_dreq pkreq;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECC_PT_DOUBLE);
   if (pkreq.status)
      return pkreq;

   struct sx_pk_inops_ecp_double inputs;
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);
   SX_PK_OP2MEM(px, inputs.px.addr, opsz);
   SX_PK_OP2MEM(py, inputs.py.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** Finish asynchronous EC point doubling.
 *
 * Get the output operands of the EC point doubling
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] rx x-coordinate of resulting point R
 * @param[out] ry y-coordinate of resulting point R
*/
static inline
void sx_async_ecp_double_end(
      sx_pk_accel *req,
      sx_op *rx,
      sx_op *ry)
{
   sx_async_finish_pair(req, rx, ry);
}


/** Compute point doubling on an elliptic curve
 *
 *  (Rx, Ry) = 2 * (Px, Py)
 *
 *
 * @param[in] curve Elliptic curve used to perform point doubling
 * @param[in] py x-coordinate of point P
 * @param[in] px y-coordinate of point P
 * @param[out] rx x-coordinate of resulting point R
 * @param[out] ry y-coordinate of resulting point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecp_double_go(), sx_async_ecp_double_end()
 * for an asynchronous verion
 */
static inline
int sx_ecp_double(
        const struct sx_pk_ecurve *curve,
        const sx_op *px,
        const sx_op *py,
        sx_op *rx,
        sx_op *ry)
{
   int status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ecp_double_go(curve, px, py);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   sx_async_ecp_double_end(pkreq.req, rx, ry);

   return status;
}


/** Asynchronous (non-blocking) EC point on curve check.
 *
 * Starts an EC point on curve check on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_PK_RELEASE_REQ()
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] px x-coordinate of point P
 * @param[in] py y-coordinate of point P
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ec_ptoncurve_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *px,
      const sx_op *py)
{
   struct sx_pk_dreq pkreq;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECC_PTONCURVE);
   if (pkreq.status)
      return pkreq;

   struct sx_pk_inops_ec_ptoncurve inputs;
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);

   SX_PK_OP2MEM(px, inputs.px.addr, opsz);
   SX_PK_OP2MEM(py, inputs.py.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** Check if the given point is on the given elliptic curve
 *
 * It succeeds if the following checks pass:
 * For GF(p):
 *  1. px < p
 *  2. py < p
 *  3. py^2 == px^3 + a * px + b mod p
 * For GF(2^m), where q = 2^m
 *  1. px < q
 *  2. py < q
 *  3. py^2 + px * py ==  px^3 + a * px^2 + b mod q
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] px x-coordinate of point P
 * @param[in] py y-coordinate of point P
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 */
static inline
int sx_ec_ptoncurve(
        const struct sx_pk_ecurve *curve,
        const sx_op *px,
        const sx_op *py)
{
   int status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ec_ptoncurve_go(curve, px, py);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   SX_PK_RELEASE_REQ(pkreq.req);

   return status;
}

/** Asynchronous (non-blocking) EC point decompression.
 *
 * Starts an EC point decompression on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ec_pt_decompression_end()
 *
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] x x-coordinate of point to decompress
 * @param[in] y_lsb Least Significant Bit of y-coordinate
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ec_pt_decompression_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *x,
      const int y_lsb)
{
   struct sx_pk_dreq pkreq;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECC_PT_DECOMP);
   if (pkreq.status)
      return pkreq;

   struct sx_pk_inops_ec_pt_decompression inputs;
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, ((y_lsb & 1) << 29),
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);

   SX_PK_OP2MEM(x, inputs.x.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}

/** Finish asynchronous (non-blocking) EC point decompression.
 *
 * Get the output operand of the EC point decompression
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] y y-coordinate of decompressed point
*/
static inline
void sx_async_ec_pt_decompression_end(
      sx_pk_accel *req,
      sx_op *y)
{
   sx_async_finish_single(req, y);
}

/** ECC point decompression
 *
 *  Recover the y coordinate of a point using
 *  x value and LSB of y:
 *    1. y = sqrt(x^3 + a * x + b)
 *    2. if (y & 1) != y_lsb then y = p - y
 *    with a and p the curve parameters
 *    3. else return ::SX_ERR_NOT_QUADRATIC_RESIDUE
 *
 * @remark: Point decompression is supported for GF(p) only
 *
 * @param[in] curve Elliptic curve used to validate point
 * @param[in] x x-coordinate of point to decompress
 * @param[in] y_lsb Least Significant Bit of y-coordinate
 * @param[out] y y-coordinate of decompressed point
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_QUADRATIC_RESIDUE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 */
static inline
int sx_ec_pt_decompression(
        const struct sx_pk_ecurve *curve,
        const sx_op *x,
        const int y_lsb,
        sx_op *y)
{
   int status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ec_pt_decompression_go(curve, x, y_lsb);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   sx_async_ec_pt_decompression_end(pkreq.req, y);

   return status;
}

/** @} */

/**  @addtogroup SX_PK_SXOPS_ECOPS
 *
 * @{
 */

/** Asynchronous (non-blocking) EC point addition.
 *
 * Start an EC point addition on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_ecp_add_end()
 *
 *
 * @param[in] curve Elliptic curve to perform EC point addition
 * @param[in] p1x x-coordinate of first point
 * @param[in] p1y y-coordinate of first point
 * @param[in] p2x x-coordinate of second point
 * @param[in] p2y y-coordinate of second point
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_async_ecp_add_go(
      const struct sx_pk_ecurve *curve,
      const sx_op *p1x,
      const sx_op *p1y,
      const sx_op *p2x,
      const sx_op *p2y)
{
   struct sx_pk_dreq pkreq;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECC_PT_ADD);
   if (pkreq.status)
      return pkreq;

   struct sx_pk_inops_ecp_add inputs;
   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0,
      (struct sx_pk_slot*)&inputs);
   if (pkreq.status)
      return pkreq;
   int opsz = SX_PK_GET_OPSIZE(pkreq.req);
   SX_PK_OP2MEM(p1x, inputs.p1x.addr, opsz);
   SX_PK_OP2MEM(p1y, inputs.p1y.addr, opsz);
   SX_PK_OP2MEM(p2x, inputs.p2x.addr, opsz);
   SX_PK_OP2MEM(p2y, inputs.p2y.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** Finish asynchronous (non-blocking) EC point addition.
 *
 * Get the output operands of the EC point addition
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] rx x-coordinate of resulting addition point
 * @param[out] ry y-coordinate of resulting addition point
 */
static inline
void sx_async_ecp_add_end(
      sx_pk_accel *req,
      sx_op *rx,
      sx_op *ry)
{
   sx_async_finish_pair(req, rx, ry);
}


/** Compute point addition on an elliptic curve
 *
 *  (Rx, Ry) = P1 + P2
 *
 * If P1 == P2 returns an SX_ERR_NOT_INVERTIBLE error
 *
 * @remark Use point doubling operation for the addition of equal points
 *
 *
 * @param[in] curve Elliptic curve to do point addition
 * @param[in] p1x x-coordinate of point P1
 * @param[in] p1y y-coordinate of point P1
 * @param[in] p2x x-coordinate of point P2
 * @param[in] p2y y-coordinate of point P2
 * @param[out] rx x-coordinate of resulting point R
 * @param[out] ry y-coordinate of resulting point R
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @remark Use point doubling operation for the addition of equal points
 * @see sx_ecp_double()
 */
static inline
int sx_ecp_ptadd(
        const struct sx_pk_ecurve *curve,
        const sx_op *p1x,
        const sx_op *p1y,
        const sx_op *p2x,
        const sx_op *p2y,
        sx_op *rx,
        sx_op *ry)
{
   int status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_async_ecp_add_go(curve, p1x, p1y, p2x, p2y);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);
   sx_async_ecp_add_end(pkreq.req, rx, ry);

   return status;
}

/** @} */

/** @} */
#ifdef __cplusplus
}
#endif

#endif
