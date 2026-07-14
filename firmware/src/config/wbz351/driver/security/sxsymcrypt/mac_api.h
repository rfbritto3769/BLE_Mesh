/** Common functions used for generation a MAC (message authentication code).
 *
 * @file
 * @copyright Copyright (c) 2020 Silex Insight. All Rights reserved.
 */

#ifndef MAC_API_FILE
#define MAC_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"


/** Feeds data to be used for MAC generation.
 *
 * The function will return immediately.
 *
 * In order to start the operation SX_MAC_GENERATE() must be called.
 *
 * @param[inout] c MAC operation context
 * @param[in] datain data to be processed, with size \p sz
 * @param[in] sz size, in bytes, of data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 * @return ::SX_ERR_FEED_COUNT_EXCEEDED
 *
 * @pre - sx_mac_create_*() function must be called first
 *
 * @remark - this function can be called even if data size, \p sz, is 0.
 * @remark - this function can be called multiple times to feed multiple chunks
 *           scattered in memory.
 */
typedef int (*FUNC_SX_MAC_FEED)(struct sxmac *c, const char *datain, size_t sz);
#define SX_MAC_FEED                                    ((FUNC_SX_MAC_FEED)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_FEED)))


/** Starts MAC generation operation.
 *
 * This function is used to start MAC generation.
 * The function will return immediately.
 *
 * The result will be transfered only after the operation is successfully
 * completed. The user shall check operation status with SX_MAC_STATUS()
 * or SX_MAC_WAIT().
 *
 * @param[inout] c MAC operation context
 * @param[out] mac generated MAC
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_SMALL
 *
 * @pre - SX_MAC_FEED() function must be called first
 *
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
typedef int (*FUNC_SX_MAC_GENERATE)(struct sxmac *c, char *mac);
#define SX_MAC_GENERATE                                ((FUNC_SX_MAC_GENERATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_GENERATE)))


/** Resumes MAC operation in context-saving.
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by SX_MAC_SAVE_STATE() in the sxmac operation
 * context \p c. It must be called with the same sxmac operation context \p c
 * that was used with SX_MAC_SAVE_STATE(). It will reserve all hardware
 * resources required to run the partial MAC operation.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions, except the sx_mac_create_*()
 * functions.
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - sx_mac_create_*() and SX_MAC_SAVE_STATE() functions
 *        must be called before, for first part of the message.
 * @pre - must be called for each part of the message(besides first), before
 *        sx_mac_crypt() or SX_MAC_SAVE_STATE().
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
typedef int (*FUNC_SX_MAC_RESUME_STATE)(struct sxmac *c);
#define SX_MAC_RESUME_STATE                            ((FUNC_SX_MAC_RESUME_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_RESUME_STATE)))


/** Starts a partial MAC operation.
 *
 * This function is used to start a partial MAC operation on data fed using
 * SX_MAC_FEED().
 * The function will return immediately.
 *
 * The partial result will be transfered only after the operation is
 * successfully completed. The user shall check operation status with
 * SX_MAC_STATUS() or SX_MAC_WAIT().
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_TOO_SMALL
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - sx_mac_crypt() should be called first.
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
typedef int (*FUNC_SX_MAC_SAVE_STATE)(struct sxmac *c);
#define SX_MAC_SAVE_STATE                              ((FUNC_SX_MAC_SAVE_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_SAVE_STATE)))


/** Waits until the given MAC generation operation has finished
 *
 * This function returns when the MAC generation operation was successfully
 * completed, or when an error has occurred that caused the operation to
 * terminate. The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_mac_create_*() is used.
 *
 * @param[inout] c MAC generation operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @see SX_MAC_STATUS().
 *
 * @remark - this function is blocking until operation finishes.
 */
typedef int (*FUNC_SX_MAC_WAIT)(struct sxmac *c);
#define SX_MAC_WAIT                                    ((FUNC_SX_MAC_WAIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_WAIT)))


/** Returns the MAC generation operation status.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the MAC generation operation has ended and all resources used by MAC generation
 * operation context \p c have been released. In this case, \p c cannot be used
 * for a new operation until one of the sx_mac_create_*() functions is
 * called again.
 *
 * @param[inout] c MAC operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - SX_MAC_FEED() and SX_MAC_GENERATE() functions must be called first
 */
typedef int (*FUNC_SX_MAC_STATUS)(struct sxmac *c);
#define SX_MAC_STATUS                                  ((FUNC_SX_MAC_STATUS)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_STATUS)))
#endif
