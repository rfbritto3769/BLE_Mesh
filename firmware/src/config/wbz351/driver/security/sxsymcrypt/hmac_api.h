/** Cryptographic HMAC(Keyed-Hash Message Authentication Code).
 *
 * The "create operation" functions are specific to HMAC. For the rest,
 * the HMAC computation is done by using the following MAC API functions:
 * SX_MAC_FEED(), SX_MAC_GENERATE(), SX_MAC_STATUS() and SX_MAC_WAIT().
 * The current implementation of HMAC does not support context-saving.
 *
 * @file
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following example shows typical sequence of function calls for computing
 * the HMAC of a message.
   @code
       SX_MAC_CREATE_HMAC_SHA256(ctx, key)
       SX_MAC_FEED(ctx, 'chunk 1')
       SX_MAC_FEED(ctx, 'chunk 2')
       SX_MAC_GENERATE(ctx)
       SX_MAC_WAIT(ctx)
   @endcode
 */

#ifndef HMAC_API_FILE
#define HMAC_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"

struct sxmac;

/** Prepares a HMAC SHA256 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] hmackey HMAC key
 * @param[in] ksz size, in bytes, of the HMAC key, can be any size
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 */
typedef int (*FUNC_SX_MAC_CREATE_HMAC_SHA256)(struct sxmac *c, struct sxkeyref *keyref);
#define SX_MAC_CREATE_HMAC_SHA2_256                      ((FUNC_SX_MAC_CREATE_HMAC_SHA256)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_HMAC_SHA2_256)))


/** Prepares a HMAC SHA384 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] hmackey HMAC key
 * @param[in] ksz size, in bytes, of the HMAC key, can be any size
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p hmackey buffer should not be changed until the operation is
 *              completed.
 */
typedef int (*FUNC_SX_MAC_CREATE_HMAC_SHA384)(struct sxmac *c, struct sxkeyref *keyref);
#define SX_MAC_CREATE_HMAC_SHA2_384                      ((FUNC_SX_MAC_CREATE_HMAC_SHA384)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_HMAC_SHA2_384)))


/** Prepares a HMAC SHA512 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] hmackey HMAC key
 * @param[in] ksz size, in bytes, of the HMAC key, can be any size
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p hmackey buffer should not be changed until the operation is
 *              completed.
 */
typedef int (*FUNC_SX_MAC_CREATE_HMAC_SHA2_512)(struct sxmac *c, struct sxkeyref *keyref);
#define SX_MAC_CREATE_HMAC_SHA2_512                      ((FUNC_SX_MAC_CREATE_HMAC_SHA2_512)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_HMAC_SHA2_512)))


/** Prepares a HMAC SHA1 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] hmackey HMAC key
 * @param[in] ksz size, in bytes, of the HMAC key, can be any size
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p hmackey buffer should not be changed until the operation is
 *              completed.
 */
typedef int (*FUNC_SX_MAC_CREATE_HMAC_SHA1)(struct sxmac *c, struct sxkeyref *keyref);
#define SX_MAC_CREATE_HMAC_SHA1                        ((FUNC_SX_MAC_CREATE_HMAC_SHA1)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_HMAC_SHA1)))


/** Prepares a HMAC SHA224 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] hmackey HMAC key
 * @param[in] ksz size, in bytes, of the HMAC key, can be any size

 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p hmackey buffer should not be changed until the operation is
 *              completed.
 */
typedef int (*FUNC_SX_MAC_CREATE_HMAC_SHA224)(struct sxmac *c, struct sxkeyref *keyref);
#define SX_MAC_CREATE_HMAC_SHA2_224                      ((FUNC_SX_MAC_CREATE_HMAC_SHA224)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_HMAC_SHA2_224)))

#endif
