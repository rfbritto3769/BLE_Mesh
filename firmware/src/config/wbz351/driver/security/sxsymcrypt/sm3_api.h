/** Cryptographic message hashing SM3.
 *
 * @file
 *
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 */

#ifndef SXSYMCRYPT_SM3_API_FILE
#define SXSYMCRYPT_SM3_API_FILE

#include "driver/security/api_table.h"

struct sxhash;


/** GM/T 0004-2012: SM3 cryptographic hash algorithm */
extern const struct sxhashalg SXHASHALG_SM3;


/** Prepares a SM3 hash operation context
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
 * @remark - SM3 digest size is 32 bytes
 */
int SX_HASH_CREATE_SM3(struct sxhash *c, size_t csz);


#endif
