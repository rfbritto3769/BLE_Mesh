/** Cryptographic message hashing SHA-2.
 *
 * @file
 *
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 */

#ifndef SXSYMCRYPT_SHA2_API_FILE
#define SXSYMCRYPT_SHA2_API_FILE

#include "driver/security/api_table.h"

struct sxhash;

/** Hash algorithm SHA-2 224
 *
 * Has only 32 bit capacity against length extension attacks.
 */
#define SXHASHALG_SHA2_224        ((const struct sxhashalg *)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SXHASHALG_SHA2_224)))


/** Hash algorithm SHA-2 256
 *
 * Has no resistance against length extension attacks.
 */
#define SXHASHALG_SHA2_256        ((const struct sxhashalg *)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SXHASHALG_SHA2_256)))

/** Hash algorithm SHA-2 384
 *
 * Has 128 bit capacity against length extension attacks.
 */
#define SXHASHALG_SHA2_384        ((const struct sxhashalg *)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SXHASHALG_SHA2_384)))

/** Hash algorithm SHA-2 512
 *
 * Has no resistance against length extension attacks.
 */
#define SXHASHALG_SHA2_512        ((const struct sxhashalg *)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SXHASHALG_SHA2_512)))

/** Prepares a SHA256 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA256 digest size is 32 bytes
 */
typedef int (*FUNC_SX_HASH_CREATE_SHA256)(struct sxhash *c, size_t csz);
#define SX_HASH_CREATE_SHA256                          ((FUNC_SX_HASH_CREATE_SHA256)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE_SHA256)))

/** Prepares a SHA384 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA384 digest size is 48 bytes
 */
typedef int (*FUNC_SX_HASH_CREATE_SHA384)(struct sxhash *c, size_t csz);
#define SX_HASH_CREATE_SHA384                          ((FUNC_SX_HASH_CREATE_SHA384)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE_SHA384)))


/** Prepares a SHA512 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA512 digest size is 64 bytes
 */
typedef int (*FUNC_SX_HASH_CREATE_SHA512)(struct sxhash *c, size_t csz);
#define SX_HASH_CREATE_SHA512                          ((FUNC_SX_HASH_CREATE_SHA512)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE_SHA512)))


/** Prepares a SHA224 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA224 digest size is 28 bytes
 */
typedef int (*FUNC_SX_HASH_CREATE_SHA224)(struct sxhash *c, size_t csz);
#define SX_HASH_CREATE_SHA224                          ((FUNC_SX_HASH_CREATE_SHA224)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE_SHA224)))


#endif
