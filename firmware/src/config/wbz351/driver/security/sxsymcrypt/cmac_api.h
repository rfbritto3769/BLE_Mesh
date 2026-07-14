/** Message Authentication Code AES CMAC.
 *
 * @file
 * @copyright Copyright (c) 2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * generating a mac.
   @code
   1. One-shot operation MAC generation
          SX_MAC_CREATE_AESCMAC(ctx, ...)
          SX_MAC_FEED(ctx, ...)
          SX_MAC_GENERATE(ctx)
          SX_MAC_WAIT(ctx)
   @endcode
 */

#ifndef CMAC_API_FILE
#define CMAC_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/sxsymcrypt/mac_api.h"
#include "driver/security/api_table.h"

/** Prepares an AES CMAC generation.
 *
 * This function initializes the user allocated object \p c with a new AES CMAC
 * operation context needed to run the CMAC generation.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the CMAC functions.
 *
 * @param[out] c CMAC operation context
 * @param[in] key key used for the CMAC generation operation,
 *                expected size 16, 24 or 32 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_MAC_CREATE_AESCMAC)(struct sxmac *c, const struct sxkeyref *key);
#define SX_MAC_CREATE_AESCMAC                          ((FUNC_SX_MAC_CREATE_AESCMAC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_MAC_CREATE_AESCMAC)))
#endif
