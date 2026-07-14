/* Copyright (c) 2019-2020 Silex Insight. All Rights reserved. */
#ifndef TRNG_INTERNAL_H
#define TRNG_INTERNAL_H

#include <stdint.h>

struct sx_regs;

/** Internal state of the TRNG hardware
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sx_trng {
    struct sx_regs *regs;
    int conditioning_key_set;
    uint32_t control;
    uint32_t key[4];
    uint32_t wakeup_lvl;
    uint32_t swoff_timer;
    uint32_t clk_divider;
    uint32_t init_wait_cnt;
};

#endif
