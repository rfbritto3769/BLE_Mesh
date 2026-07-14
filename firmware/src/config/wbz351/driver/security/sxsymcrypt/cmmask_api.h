/** AES counter-measures mask load
 *
 * @file
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following example shows typical sequence of function calls for
 * loading the counter-measure mask.
   @code
       SX_CM_LOAD_MASK(ctx, value)
       SX_CM_LOAD_MASK_WAIT(ctx)
   @endcode
 */

#ifndef CMMASK_API_FILE
#define CMMASK_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"


/** Loads random used in the AES counter-measures.
 *
 * Counter-measures are available for the AES, if enabled in HW.
 * This function initializes the user allocated object \p c with a new
 * counter-measures mask load operation context and reserves the HW resource.
 *
 * After the initialization of \p c, this function starts the load of the
 * counter-measure cryptographically secure random \p value.
 * The function will return immediately. No data will be received for the
 * load operation.
 *
 * The operation is considered successful if the status returned by
 * SX_CM_LOAD_MASK_WAIT() or SX_CM_LOAD_MASK_STATUS() is ::SX_OK.
 *
 * @param[out] c counter-measures mask load operation context
 * @param[in] value counter-measures random value to be loaded
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - it is under the user responsibility to call it after system boot
 *           (not automatically called).
 * @remark - for more details see the technical report: "Secure and Efficient
 *           Masking of AES - A Mission Impossible?", June 2004)
 */
typedef int (*FUNC_SX_CM_LOAD_MASK)(struct sxcmmask *c, uint32_t value);
#define SX_CM_LOAD_MASK                                ((FUNC_SX_CM_LOAD_MASK)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_CM_LOAD_MASK)))


/** Waits until the given AES counter-measures load operation has finished.
 *
 * This function returns when the counter-measures load operation was
 * successfully completed, or when an error has occurred that caused the
 * operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless SX_CM_LOAD_MASK() is used.
 *
 * @param[inout] c counter-measures mask load operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - SX_CM_LOAD_MASK() function must be called first
 *
 * @see SX_CM_LOAD_MASK_STATUS().
 *
 * @remark - this function is blocking until operation finishes.
 */
typedef int (*FUNC_SX_CM_LOAD_MASK_WAIT)(struct sxcmmask *c);
#define SX_CM_LOAD_MASK_WAIT                           ((FUNC_SX_CM_LOAD_MASK_WAIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_CM_LOAD_MASK_WAIT)))


/** Returns the status of the given AES counter-measures load operation context.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the counter-measures mask load operation has ended and all resources used by
 * counter-measures mask load operation context \p c have been released. In this
 * case, \p c cannot be used for a new operation until SX_CM_LOAD_MASK() is
 * called again.
 *
 * @param[inout] c counter-measures mask load operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - SX_CM_LOAD_MASK() function must be called first
 */
typedef int (*FUNC_SX_CM_LOAD_MASK_STATUS)(struct sxcmmask *c);
#define SX_CM_LOAD_MASK_STATUS                         ((FUNC_SX_CM_LOAD_MASK_STATUS)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_CM_LOAD_MASK_STATUS)))
#endif
