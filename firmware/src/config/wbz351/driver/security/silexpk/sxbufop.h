/** Basic "sxops" operand definition
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering scs
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADAPTER_TYPES_HEADER
#define ADAPTER_TYPES_HEADER

#include <stddef.h>
#include <stdint.h>

/** Basic operand representation **/
struct sx_buf {
     /** Size in bytes of operand **/
     size_t sz;
     /** Memory of operand bytes in big endian **/
     char *bytes;
};
/** Simple "sxops" implementation based on sx_buf**/
typedef struct sx_buf sx_op;

#endif
