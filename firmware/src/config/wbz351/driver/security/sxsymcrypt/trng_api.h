/** True Random Number Generator (TRNG)
 *
 * @file
 * @copyright Copyright (c) 2020 Silex Insight. All Rights reserved.
 *
 * The TRNG uses the hardware to generate random number with a high
 * level of entropy. It's intended to feed entropy into a DRBG.
 */

#ifndef TRNG_API_H
#define TRNG_API_H

#include <stdint.h>
#include <stddef.h>
#include "driver/security/sxsymcrypt/trnginternal.h"
#include "driver/security/api_table.h"

/** Configuration parameters for the TRNG */
struct sx_trng_config {
    /** FIFO level below which the module leaves the idle state to refill the FIFO
     * 
     * In numbers of 128-bit blocks.
     * 
     * Set to 0 to use default.
     */
    unsigned int wakeup_level;
    
    /** Number of clock cycles to wait before sampling data from the noise source
     * 
     * Set to 0 to use default.
     */
    unsigned int init_wait;
    
    /** Number of clock cycles to wait before stopping the rings after the FIFO is full.
     * 
     * Set to 0 to use default.
     */
    unsigned int off_time_delay;
    
    /** Clock divider for the frequency at which the outputs of the rings are sampled.
     * 
     * Set to 0 to sample at APB interface clock frequency.
     */
    unsigned int sample_clock_div;
};

/** TRNG initialization
 *
 * @param[inout] ctx TRNG context to be used in other operations
 * @param[in] config pointer to optional configuration. NULL to use default.
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 */
typedef int (*FUNC_SX_TRNG_INIT)(struct sx_trng *ctx, const struct sx_trng_config *config);
#define SX_TRNG_INIT                              ((FUNC_SX_TRNG_INIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_TRNG_INIT)))

/** Get random bytes
 *
 * When this function returns ::SX_OK, \p size random bytes have been
 * written to the \p dst memory location. If not enough random bytes are
 * available, the function does not write any data to \p dst and returns
 * ::SX_ERR_INSUFFICIENT.
 *
 * @param[inout] ctx TRNG context to be used in other operations
 * @param[out] dst Destination in memory to copy \p size bytes to
 * @param[in] size length in bytes
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INSUFFICIENT
 * @return ::SX_ERR_HARDWARE_FAILURE
 */
typedef int (*FUNC_SX_TRNG_GET)(struct sx_trng *ctx, char *dst, size_t size);
#define SX_TRNG_GET                                    ((FUNC_SX_TRNG_GET)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_TRNG_GET)))

#endif
