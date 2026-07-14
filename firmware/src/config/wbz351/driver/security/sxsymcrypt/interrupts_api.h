/** Hardware interrupts
 *
 * @file
 * @copyright Copyright (c) 2020 Silex Insight. All Rights reserved.
 */

#ifndef INTERRUPTS_API_FILE
#define INTERRUPTS_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"

/** Prepares the hardware to use hardware interrupts.
 *
 * This function may be called only once, before any function that starts an
 * aead, blkcipher, or hash operation.
 *
 * @return ::SX_OK
 *
 * @remark - hardware interrupts are not available for cmmask.
 */
typedef int (*FUNC_SX_INTERRUPTS_ENABLE)(void);
#define SX_INTERRUPTS_ENABLE                           ((FUNC_SX_INTERRUPTS_ENABLE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_INTERRUPTS_ENABLE)))

/** Disables all hardware interrupts.
 *
 * This function may be called only when there is no ongoing hardware
 * processing.
 *
 * @return ::SX_OK
 */
typedef int (*FUNC_SX_INTERRUPTS_DISABLE)(void);
#define SX_INTERRUPTS_DISABLE                          ((FUNC_SX_INTERRUPTS_DISABLE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_INTERRUPTS_DISABLE)))
#endif
