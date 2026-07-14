/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef INTERNAL_HEADER_FILE_PK
#define INTERNAL_HEADER_FILE_PK

#include <stdint.h>

struct sx_pk_cmd_def;

struct sx_regs
{
   char *base;
};

struct sx_pk_accel
{
   struct sx_regs regs;
   char *cryptoram;
   int slot_sz;
   int op_size;
   const struct sx_pk_cmd_def *cmd;
   struct sx_pk_cnx *cnx;
   const char *outputops[12];
   void *userctxt;
   int ik_mode;
};

#define NUM_PK_INST     1

struct sx_pk_cnx {
   struct sx_pk_accel instances[NUM_PK_INST];
   struct sx_pk_capabilities caps;
};

int sx_pk_open_extra(struct sx_pk_cnx *cnx, struct sx_pk_config *cfg);

void sx_pk_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t v);

uint32_t sx_pk_rdreg(struct sx_regs *regs, uint32_t addr);

#endif
