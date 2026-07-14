/** "sxops" interface for DSA cryptographic computations
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
 * Copyright (c) 2020 Silex Insight sa
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef DSA_HEADER_FILE
#define DSA_HEADER_FILE

#include <driver/security/silexpk/core_api.h>
#include "driver/security/api_table.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/security/silexpk/adapter_api.h"
#include "driver/security/silexpk/impl.h"
#include <driver/security/silexpk/inputslots.h>
#include <driver/security/silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_COMPATIBLE(1, 1);

struct sx_pk_cmd_def;

/**
 * @addtogroup SX_PK_SXOPS_DSA
 *
 * @{
 */

/** Asynchronous (non-blocking) DSA signature generation
 *
 * Start an DSA signature generation on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call sx_async_finish_pair()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] k Random value
 * @param[in] privkey Private key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_dsa_sign() for a synchronous version
 */
static inline
struct sx_pk_dreq sx_async_dsa_sign_go(struct sx_pk_cnx *cnx,
   const sx_op *p, const sx_op *q, const sx_op *g, const sx_op *k,
   const sx_op *privkey, const sx_op *h)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_dsa_sign inputs;

   pkreq = SX_PK_ACQUIRE_REQ(cnx, SX_PK_CMD_DSA_SIGN);
   if (pkreq.status)
      return pkreq;

   // convert and transfer operands
   int sizes[] = {
      SX_OP_SIZE(p),
      SX_OP_SIZE(q),
      SX_OP_SIZE(g),
      SX_OP_SIZE(k),
      SX_OP_SIZE(privkey),
      SX_OP_SIZE(h),
   };
   pkreq.status = SX_PK_LIST_GFP_INSLOTS(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;
   SX_PK_OP2VMEM(p, inputs.p.addr);
   SX_PK_OP2VMEM(q, inputs.q.addr);
   SX_PK_OP2VMEM(g, inputs.g.addr);
   SX_PK_OP2VMEM(k, inputs.k.addr);
   SX_PK_OP2VMEM(privkey, inputs.privkey.addr);
   SX_PK_OP2VMEM(h, inputs.h.addr);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** DSA signature generation
 *
 * Computes the following:
 *    1. X = g ^ k mod p
 *    2. r = X mod q
 *    3. if r == 0 the return ::SX_ERR_INVALID_SIGNATURE
 *    4. else w = k ^ (-1) mod q
 *    5. s = w * (h + x * r) mod q
 *    6. if s == 0 then return ::SX_ERR_INVALID_SIGNATURE
 *    7. (r,s) is the signature
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] k Random value
 * @param[in] privkey Private key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 * @param[out] r First part of signature
 * @param[out] s Second part of signature
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_dsa_sign_go() for an asynchronous version
 */
int sx_dsa_sign(struct sx_pk_cnx *cnx,
   const sx_op *p, const sx_op *q, const sx_op *g, const sx_op *k,
   const sx_op *privkey, const sx_op *h, sx_op *r, sx_op *s)
{
   struct sx_pk_dreq pkreq;
   int status;

   pkreq = sx_async_dsa_sign_go(cnx, p, q, g, k, privkey, h);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);

   sx_async_finish_pair(pkreq.req, r, s);

   return status;
}


/** Asynchronous (non-blocking) DSA signature verification
 *
 * Start an DSA signature verification on the accelerator
 * and return immediately.
 *
 * @remark When the operation finishes on the accelerator,
 * call SX_PK_RELEASE_REQ()
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] pubkey Public key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 * @param[in] r First part of the signature to verify
 * @param[in] s Second part of the signature to verify
 *
 * @return Acquired acceleration request for this operation
 *
 * @see sx_dsa_ver() for a synchronous version
 */
static inline
struct sx_pk_dreq sx_async_dsa_ver_go(struct sx_pk_cnx *cnx,
   const sx_op *p, const sx_op *q, const sx_op *g, const sx_op *pubkey,
   const sx_op *h, const sx_op *r, const sx_op *s)
{
   struct sx_pk_dreq pkreq;
   struct sx_pk_inops_dsa_ver inputs;

   pkreq = SX_PK_ACQUIRE_REQ(cnx, SX_PK_CMD_DSA_VER);
   if (pkreq.status)
      return pkreq;

   // convert and transfer operands
   int sizes[] = {
      SX_OP_SIZE(p),
      SX_OP_SIZE(q),
      SX_OP_SIZE(g),
      SX_OP_SIZE(pubkey),
      SX_OP_SIZE(h),
      SX_OP_SIZE(r),
      SX_OP_SIZE(s),
   };
   pkreq.status = SX_PK_LIST_GFP_INSLOTS(pkreq.req, sizes, (struct sx_pk_slot *)&inputs);
   if (pkreq.status)
      return pkreq;
   SX_PK_OP2VMEM(p, inputs.p.addr);
   SX_PK_OP2VMEM(q, inputs.q.addr);
   SX_PK_OP2VMEM(g, inputs.g.addr);
   SX_PK_OP2VMEM(pubkey, inputs.pubkey.addr);
   SX_PK_OP2VMEM(h, inputs.h.addr);
   SX_PK_OP2VMEM(r, inputs.r.addr);
   SX_PK_OP2VMEM(s, inputs.s.addr);

   SX_PK_RUN(pkreq.req);

   return pkreq;
}


/** DSA signature verification
 *
 * Checks if a signature is valid:
 *    1. w = s ^ (-1) mod q
 *    2. u1 = h * w mod q
 *    3. u2 = r * w mod q
 *    4. X = g ^ (u1) * y ^ (u2) mod p
 *    5. v = X mod q
 *    6. if v == r then signature is valid (::SX_OK)
 *    7. else return ::SX_ERR_INVALID_SIGNATURE
 *
 * Before launching the operation, verify the domain D(p,q,g)
 * by checking:
 *    1. 2^1023 < p < 2^1024 \b or 2^2047 < p < 2^2048
 *    2. 2^159 < q < 2^160 \b or 2^223 < q < 2^224 \b or 2^255 < q < 2^256
 *    3. 1 < g < p
 *
 * @param[inout] cnx Connection structure obtained through SX_PK_OPEN() at startup
 * @param[in] p Prime modulus p
 * @param[in] q Prime divisor of p-1
 * @param[in] g Generator of order q mod p
 * @param[in] pubkey Public key
 * @param[in] h Hash digest of message reduced by means of
 * Secure Hash Algorithm specified in FIPS 180-3
 * @param[in] r First part of the signature to verify
 * @param[in] s Second part of the signature to verify
 *
 * @return ::SX_OK
 * @return ::SX_ERR_NOT_INVERTIBLE
 * @return ::SX_ERR_INVALID_SIGNATURE
 * @return ::SX_ERR_OUT_OF_RANGE
 * @return ::SX_ERR_INVALID_PARAM
 * @return ::SX_ERR_UNKNOWN_ERROR
 * @return ::SX_ERR_BUSY
 * @return ::SX_ERR_NOT_IMPLEMENTED
 * @return ::SX_ERR_OPERAND_TOO_LARGE
 * @return ::SX_ERR_PLATFORM_ERROR
 * @return ::SX_ERR_EXPIRED
 *
 * @see sx_async_dsa_ver_go() for an asynchronous version
 */
int sx_dsa_ver(struct sx_pk_cnx *cnx,
   const sx_op *p, const sx_op *q, const sx_op *g, const sx_op *pubkey,
   const sx_op *h, const sx_op *r, const sx_op *s)
{
   struct sx_pk_dreq pkreq;
   int status;

   pkreq = sx_async_dsa_ver_go(cnx, p, q, g, pubkey, h, r, s);
   if (pkreq.status)
      return pkreq.status;

   status = SX_PK_WAIT(pkreq.req);

   SX_PK_RELEASE_REQ(pkreq.req);

   return status;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
