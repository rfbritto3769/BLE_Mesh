/** "sxops" interface for EC J-PAKE computations.
 *
 * Simpler functions to perform public key crypto operations. Included directly
 * in some interfaces (like sxbuf or OpenSSL engine). The functions
 * take input operands (large integers) and output operands
 * which will receive the computed results.
 *
 * Operands have the "sx_op" type. The specific interfaces (like sxbuf) define
 * the "sx_op" type.
 *
 * @file
 */
/*
 * Copyright (c) 2020 Silex Insight sa
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ECJPAKE_HEADER_FILE
#define ECJPAKE_HEADER_FILE

#include "driver/security/silexpk/core_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/security/silexpk/inputslots.h"
#include "driver/security/silexpk/iomem.h"
#include "driver/security/silexpk/ec_curves_api.h"
#include "driver/security/silexpk/adapter_api.h"
#include "driver/security/silexpk/impl.h"
#include "driver/security/silexpk/version.h"

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_COMPATIBLE(1, 3);

struct sx_pk_ecurve;

/**
 * @addtogroup SX_PK_SXOPS_ECJPAKE
 *
 * @{
 */

/** Curve generator point for sx_ecjpake_verify_zkp() or sx_ecjpake_verify_zkp_go() */
#ifndef SX_PT_CURVE_GENERATOR
#define SX_PT_CURVE_GENERATOR NULL
#endif

/** Affine point parameter group
 *
 * This structure is used for point values which are stored in
 * two consecutive locations (x and y).
*/
struct sx_pk_point {
   sx_op *x; /**< x-coordinate */
   sx_op *y; /**< y-coordinate */
};

/** Asynchronous EC J-PAKE proof generation
 *
 * Start a EC J-PAKE proof generation operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_generate_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Random input (< n)
 * @param[in] x Exponent
 * @param[in] h Hash digest
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_ecjpake_generate_zkp_go(
   const struct sx_pk_ecurve *curve,
   const sx_op *v,
   const sx_op *x,
   const sx_op *h)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecjpake_generate_zkp inputs;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECJPAKE_GENERATE_ZKP);
   if (pkreq.status)
      return pkreq;

   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;

   int opsz = SX_PK_GET_OPSIZE(pkreq.req);

   SX_PK_OP2MEM(v, inputs.v.addr, opsz);
   SX_PK_OP2MEM(x, inputs.x.addr, opsz);
   SX_PK_OP2MEM(h, inputs.h.addr, opsz);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE proof generation.
 *
 * Get the output operands of the EC J-PAKE proof generation
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] r The resulting value
 */
static inline
void sx_ecjpake_generate_zkp_end(
   sx_pk_accel *req,
   sx_op *r)
{
   sx_async_finish_single(req, r);
}

/** Perform an EC J-PAKE proof generation
 *
 * The proof generation has the following steps:
 *   1. r = (v - (x * h)) % n [where n is the modulus, a curve parameter]
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Random input (< n)
 * @param[in] x Exponent
 * @param[in] h Hash digest
 *
 * Truncation or padding should be done by user application
 * @param[out] r The result
 *
 * @return ::SX_OK
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecjpake_generate_zkp_go(), sx_async_ecjpake_generate_zkp_end() for
 * an asynchronous version
 */
static inline
int sx_ecjpake_generate_zkp(
   const struct sx_pk_ecurve *curve,
   const sx_op *v,
   const sx_op *x,
   const sx_op *h,
   sx_op *r
   )
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_ecjpake_generate_zkp_go(curve, v, x, h);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);

   sx_ecjpake_generate_zkp_end(pkreq.req, r);

   return status;
}

/** Asynchronous EC J-PAKE proof verification
 *
 * Start an EC J-PAKE proof verification operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_verify_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Point on the curve
 * @param[in] x Point on the curve
 * @param[in] r Proof to be verified
 * @param[in] h Hash digest
 * @param[in] g Point on the curve (Optional, pass SX_PT_CURVE_GENERATOR to use
 *              the curve generator point)
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_ecjpake_verify_zkp_go(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *v,
   const struct sx_pk_point *x,
   const sx_op *r,
   const sx_op *h,
   const struct sx_pk_point *g)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecjpake_verify_zkp inputs;

   pkreq = SX_PK_ACQUIRE_REQ(curve->cnx, SX_PK_CMD_ECJPAKE_VERIFY_ZKP);
   if (pkreq.status)
      return pkreq;

   pkreq.status = SX_PK_LIST_ECC_INSLOTS(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;

   int opsz = SX_PK_GET_OPSIZE(pkreq.req);

   sx_pk_op2mem(v->x, inputs.xv.addr, opsz);
   sx_pk_op2mem(v->y, inputs.yv.addr, opsz);
   sx_pk_op2mem(x->x, inputs.xx.addr, opsz);
   sx_pk_op2mem(x->y, inputs.yx.addr, opsz);
   sx_pk_op2mem(r, inputs.r.addr, opsz);
   sx_pk_op2mem(h, inputs.h.addr, opsz);

   if (g != SX_PT_CURVE_GENERATOR) {
      sx_pk_op2mem(g->x, inputs.xg2.addr, opsz);
      sx_pk_op2mem(g->y, inputs.yg2.addr, opsz);
   } else {
      sx_pk_write_curve_gen(pkreq.req, curve, inputs.xg2, inputs.yg2);
   }

   SX_PK_RUN(pkreq.req);

   return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE proof verification.
 *
 * Finishes the EC J-PAKE proof verification
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 */
static inline
void sx_ecjpake_verify_zkp_end(
   sx_pk_accel *req
   )
{
   sx_pk_release_req(req);
}

/** Synchronous EC J-PAKE proof verification
 *
 * Start an EC J-PAKE proof verification operation on the accelerator
 * and return immediately.
 *
 * The proof verification has the following steps:
 *   1. ( (G * r) + (X * h) ) ?= V
 *
 * In case of a comparison failure SX_ERR_INVALID_SIGNATURE shall
 * be returned.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_verify_zkp_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] v Point on the curve
 * @param[in] x Point on the curve
 * @param[in] r Proof to be verified
 * @param[in] h Hash digest
 * @param[in] g Point on the curve (Optional, pass SX_PT_CURVE_GENERATOR to use
 *              the curve generator point)
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecjpake_verify_zkp_go(), sx_async_ecjpake_verify_zkp_end() for
 * an asynchronous version
 */
static inline
int sx_ecjpake_verify_zkp(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *v,
   const struct sx_pk_point *x,
   const sx_op *r,
   const sx_op *h,
   const struct sx_pk_point *g
   )
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_ecjpake_verify_zkp_go(curve, v, x, r, h, g);
   if (pkreq.status)
      return pkreq.status;

   status = sx_pk_wait(pkreq.req);

   sx_ecjpake_verify_zkp_end(pkreq.req);

   return status;
}

/** Asynchronous EC J-PAKE 3 point addition
 *
 * Start a EC J-PAKE 3 point addition operation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_3pt_add_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] a Point on the curve
 * @param[in] b Point on the curve
 * @param[in] c Point on the curve
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 *
 */
static inline
struct sx_pk_dreq sx_ecjpake_3pt_add_go(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *a,
   const struct sx_pk_point *b,
   const struct sx_pk_point *c)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecjpake_3pt_add inputs;

   pkreq = sx_pk_acquire_req(curve->cnx, SX_PK_CMD_ECJPAKE_3PT_ADD);
   if (pkreq.status)
      return pkreq;

   pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;

   int opsz = sx_pk_get_opsize(pkreq.req);

   sx_pk_op2mem(b->x, inputs.x2_1.addr, opsz);
   sx_pk_op2mem(b->y, inputs.x2_2.addr, opsz);
   sx_pk_op2mem(c->x, inputs.x3_1.addr, opsz);
   sx_pk_op2mem(c->y, inputs.x3_2.addr, opsz);
   sx_pk_op2mem(a->x, inputs.x1_1.addr, opsz);
   sx_pk_op2mem(a->y, inputs.x1_2.addr, opsz);

   sx_pk_run(pkreq.req);

   return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE 3 point addition.
 *
 * Finishes the EC J-PAKE 3 point addition
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] gb The addition result
 */
static inline
void sx_ecjpake_3pt_add_end(
   sx_pk_accel *req,
   struct sx_pk_point *gb
   )
{
   sx_async_finish_pair(req, gb->x, gb->y);
}

/** Synchronous EC J-PAKE 3 point addition
 *
 * Start a EC J-PAKE 3 point addition operation on the accelerator
 * and return immediately.
 *
 * The 3 point addition operation has the following steps:
 *   1. gb = a + b + c
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_3pt_add_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] a Point on the curve
 * @param[in] b Point on the curve
 * @param[in] c Point on the curve
 * @param[out] gb The addition result
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecjpake_verify_zkp_go(), sx_async_ecjpake_verify_zkp_end() for
 * an asynchronous version
 */
static inline
int sx_ecjpake_3pt_add(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *a,
   const struct sx_pk_point *b,
   const struct sx_pk_point *c,
   struct sx_pk_point *gb
   )
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_ecjpake_3pt_add_go(curve, a,b,c);
   if (pkreq.status)
      return pkreq.status;

   status = sx_pk_wait(pkreq.req);

   sx_ecjpake_3pt_add_end(pkreq.req, gb);

   return status;
}

/** Asynchronous EC J-PAKE session key generation
 *
 * Start a EC J-PAKE session key generation operation on
 * the accelerator and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_sess_key_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] b Point on the curve
 * @param[in] x2 Generated random number
 * @param[in] x2s (x2 * s) % n [s = password, n = modulus]
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_ecjpake_gen_sess_key_go(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *x4,
   const struct sx_pk_point *b,
   const sx_op *x2,
   const sx_op *x2s)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecjpake_gen_sess_key inputs;

   pkreq = sx_pk_acquire_req(curve->cnx, SX_PK_CMD_ECJPAKE_GEN_SESS_KEY);
   if (pkreq.status)
      return pkreq;

   pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;

   int opsz = sx_pk_get_opsize(pkreq.req);

   sx_pk_op2mem(x4->x, inputs.x4_1.addr, opsz);
   sx_pk_op2mem(x4->y, inputs.x4_2.addr, opsz);
   sx_pk_op2mem(b->x, inputs.b_1.addr, opsz);
   sx_pk_op2mem(b->y, inputs.b_2.addr, opsz);
   sx_pk_op2mem(x2, inputs.x2.addr, opsz);
   sx_pk_op2mem(x2s, inputs.x2s.addr, opsz);

   sx_pk_run(pkreq.req);

   return pkreq;
}

/** Finish asynchronous (non-blocking) EC J-PAKE session key generation
 *
 * Finishes the EC J-PAKE session key generation operation
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] t The result
 */
static inline
void sx_ecjpake_gen_sess_key_end(
   sx_pk_accel *req,
   struct sx_pk_point *t)
{
   sx_async_finish_pair(req, t->x, t->y);
}

/** Synchronous EC J-PAKE session key generation
 *
 * Start a EC J-PAKE session key generation operation on the accelerator
 * and return immediately.
 *
 * The session key generation has the following steps:
 *   1. T = (b - (x4 * x2s)) * x2
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_sess_key_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] b Point on the curve
 * @param[in] x2 Generated random number
 * @param[in] x2s x2 * password
 * @param[out] t result
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecjpake_gen_sess_key_go(), sx_async_ecjpake_gen_sess_key_end() for
 * an asynchronous version
 */
static inline
int sx_ecjpake_gen_sess_key(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *x4,
   const struct sx_pk_point *b,
   const sx_op *x2,
   const sx_op *x2s,
   struct sx_pk_point *t
   )
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_ecjpake_gen_sess_key_go(curve, x4, b, x2, x2s);
   if (pkreq.status)
      return pkreq.status;

   status = sx_pk_wait(pkreq.req);

   sx_ecjpake_gen_sess_key_end(pkreq.req, t);

   return status;
}

/** Asynchronous EC J-PAKE step 2 calculation
 *
 * Start an EC J-PAKE step 2 calculation operation on
 * the accelerator and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_ecjpake_gen_step_2_end()
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] x3 Point on the curve
 * @param[in] x1 Point on the curve
 * @param[in] x2s (x2 * s) % n [x2 = random, s = password, n = modulus]
 * @param[in] s password
 *
 * Truncation or padding should be done by user application
 *
 * @return Acquired acceleration request for this operation
 */
static inline
struct sx_pk_dreq sx_ecjpake_gen_step_2_go(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *x4,
   const struct sx_pk_point *x3,
   const struct sx_pk_point *x1,
   const sx_op *x2s,
   const sx_op *s)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_ecjpake_gen_step_2 inputs;

   pkreq = sx_pk_acquire_req(curve->cnx, SX_PK_CMD_ECJPAKE_GEN_STEP_2);
   if (pkreq.status)
      return pkreq;

   pkreq.status = sx_pk_list_ecc_inslots(pkreq.req, curve, 0, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;

   int opsz = sx_pk_get_opsize(pkreq.req);

   sx_pk_op2mem(x4->x, inputs.x4_1.addr, opsz);
   sx_pk_op2mem(x4->y, inputs.x4_2.addr, opsz);
   sx_pk_op2mem(x3->x, inputs.x3_1.addr, opsz);
   sx_pk_op2mem(x3->y, inputs.x3_2.addr, opsz);
   sx_pk_op2mem(x1->x, inputs.x1_1.addr, opsz);
   sx_pk_op2mem(x1->y, inputs.x1_2.addr, opsz);
   sx_pk_op2mem(x2s, inputs.x2s.addr, opsz);
   sx_pk_op2mem(s, inputs.s.addr, opsz);

   sx_pk_run(pkreq.req);

   return pkreq;
}

/** Finish an asynchronous (non-blocking) EC J-PAKE step 2 calculation
 *
 * Finishes the EC J-PAKE step 2 calculation operation
 * and releases the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[inout] req The previously acquired acceleration
 * request for this operation
 * @param[out] a Point on the curve
 * @param[out] x2s Generated random * password
 * @param[out] ga Point on the curve
 */
static inline
void sx_ecjpake_gen_step_2_end(
   sx_pk_accel *req,
   struct sx_pk_point *a,
   sx_op *x2s,
   struct sx_pk_point *ga)
{
   sx_op *results[5] = { a->x, a->y, x2s, ga->x, ga->y };

   sx_async_finish_any(req, results, 5);
}

/** Synchronous EC J-PAKE step 2 calculation
 *
 * Start an EC J-PAKE step 2 calculation operation on the accelerator
 * and return immediately.
 *
 * The step 2 calculation has the following steps:
 *   1. ga = x1 + x3 + x4
 *   2. rx2s = (x2s * s) % curve.n
 *   3. a = ga * rx2s
 *
 * @param[in] curve Elliptic curve on which to perform the operation.
 * @param[in] x4 Point on the curve
 * @param[in] x3 Point on the curve
 * @param[in] x1 Point on the curve
 * @param[in] x2s Generated random * password
 * @param[in] s Password
 * @param[out] a Point on the curve
 * @param[out] rx2s Generated random * password
 * @param[out] ga Point on the curve
 *
 * Truncation or padding should be done by user application
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_POINT_NOT_ON_CURVE
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_ecjpake_gen_step_2_go(), sx_async_ecjpake_gen_step_2_end() for
 * an asynchronous version
 */
static inline
int sx_ecjpake_gen_step_2(
   const struct sx_pk_ecurve *curve,
   const struct sx_pk_point *x4,
   const struct sx_pk_point *x3,
   const struct sx_pk_point *x1,
   const sx_op *x2s,
   const sx_op *s,
   struct sx_pk_point *a,
   sx_op *rx2s,
   struct sx_pk_point *ga
   )
{
   uint32_t status;
   struct sx_pk_dreq pkreq;

   pkreq = sx_ecjpake_gen_step_2_go(curve, x4, x3, x1, x2s, s);
   if (pkreq.status)
      return pkreq.status;

   status = sx_pk_wait(pkreq.req);

   sx_ecjpake_gen_step_2_end(pkreq.req, a, rx2s, ga);

   return status;
}

/** @} */
#ifdef __cplusplus
}
#endif

#endif
