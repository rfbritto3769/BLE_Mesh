/** Cryptographic message hashing SHA-1.
 *
 * @file
 *
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 */

#ifndef SXSYMCRYPT_SHA1_API_FILE
#define SXSYMCRYPT_SHA1_API_FILE

#include "driver/security/api_table.h"

struct sxhash;


/** Hash algorithm SHA-1 (Secure Hash Algorithm 1)
 *
 * Deprecated algorithm. NIST formally deprecated use of SHA-1 in 2011
 * and disallowed its use for digital signatures in 2013. SHA-3 or SHA-2
 * are recommended instead.
 */
#define SXHASHALG_SHA1        ((const struct sxhashalg *)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SXHASHALG_SHA1)))


/** Prepares a SHA1 hash operation context
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
 * @remark - SHA1 digest size is 20 bytes
 */
typedef int (*FUNC_SX_HASH_CREATE_SHA1)(struct sxhash *c, size_t csz);
#define SX_HASH_CREATE_SHA1                            ((FUNC_SX_HASH_CREATE_SHA1)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE_SHA1)))

#endif
