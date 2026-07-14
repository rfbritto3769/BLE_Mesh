/** "sxops" interface to finalise acceleration requests
 * and get the output operands
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef SXOPS_IMPL_HEADER_FILE
#define SXOPS_IMPL_HEADER_FILE

#include <driver/security/silexpk/version.h>

/** Make sure the application is compatible with SilexPK API version **/
SX_PK_API_ASSERT_COMPATIBLE(1, 0);

/** Finish an operation with one result operands
 *
 * Write the single result in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where an
 * operation with 1 output operands has finished
 * @param[out] result Result operand of a single output operand operation
 */
static inline
void sx_async_finish_single(sx_pk_accel *pkhw, sx_op *result)
{
   const char **outputs = SX_PK_GET_OUTPUT_OPS(pkhw);
   const int opsz = SX_PK_GET_OPSIZE(pkhw);

   SX_PK_MEM2OP(outputs[0], opsz, result);

   SX_PK_RELEASE_REQ(pkhw);
}

/** Finish an operation with two result operands
 *
 * Write the two results in the result operand
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 2 output operands has finished
 * @param[out] r1 First result operand of the operation
 * @param[out] r2 Second result operand of the operation
 */
static inline
void sx_async_finish_pair(sx_pk_accel *pkhw, sx_op *r1, sx_op *r2)
{
   const char **outputs = SX_PK_GET_OUTPUT_OPS(pkhw);
   const int opsz = SX_PK_GET_OPSIZE(pkhw);

   SX_PK_MEM2OP(outputs[0], opsz, r1);
   SX_PK_MEM2OP(outputs[1], opsz, r2);

   SX_PK_RELEASE_REQ(pkhw);
}

/** Finish an operation with four result operands
 *
 * Write the four results in the result operand buffers
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 4 resulting operands has finished
 * @param[out] r1 First result operand of the operation
 * @param[out] r2 Second result operand of the operation
 * @param[out] r3 Third result operand of the operation
 * @param[out] r4 Fourth result operand of the operation
 */
static inline
void sx_async_finish_quad(sx_pk_accel *pkhw, sx_op *r1, sx_op *r2, sx_op *r3, sx_op *r4)
{
   const char **outputs = SX_PK_GET_OUTPUT_OPS(pkhw);
   const int opsz = SX_PK_GET_OPSIZE(pkhw);

   SX_PK_MEM2OP(outputs[0], opsz, r1);
   SX_PK_MEM2OP(outputs[1], opsz, r2);
   SX_PK_MEM2OP(outputs[2], opsz, r3);
   SX_PK_MEM2OP(outputs[3], opsz, r4);

   SX_PK_RELEASE_REQ(pkhw);
}

/** Finish an operation with any number of result operands
 *
 * Write results in the result operand buffer
 * and release the reserved resources.
 *
 * @pre The operation on the accelerator must be finished before
 * calling this function.
 *
 * @param[in] pkhw The acceleration request where the
 * operation with 'count' resulting operands has finished
 * @param[out] results Buffer of result operands of the operation
 * @param[in] count The number of result operands
 */
static inline
void sx_async_finish_any(sx_pk_accel *pkhw, sx_op **results, int count)
{
   const char **outputs = SX_PK_GET_OUTPUT_OPS(pkhw);
   const int opsz = SX_PK_GET_OPSIZE(pkhw);

   for (int i=0; i < count; i++)
      SX_PK_MEM2OP(outputs[i], opsz, results[i]);
   SX_PK_RELEASE_REQ(pkhw);
}
#endif
