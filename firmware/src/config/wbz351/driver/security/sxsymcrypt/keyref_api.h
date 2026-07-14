/** Common function definitions for keys.
 *
 * @file
 * Copyright (c) 2020 Silex Insight. All Rights reserved.
 */

#ifndef KEYREF_API_FILE
#define KEYREF_API_FILE

#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"


/** Returns a reference to a key whose key material is in user memory.
 *
 * This function loads the user provided key data and returns an initialized
 * sxkeyref object.
 *
 * The returned object can be passed to any of the sx_aead_create_*() or
 * sx_blkcipher_create_*() functions.
 *
 * @param[in] keysz size of the key to be loaded
 * @param[in] keymaterial key to be loaded with size \p keysz
 * @return sxkeyref initialized object with provided inputs
 *
 * @remark - \p keymaterial buffer should not be changed until the operation
 *           is completed.
 */
typedef struct sxkeyref (*FUNC_SX_KEYREF_LOAD_MATERIAL)(size_t keysz, const char *keymaterial);
#define SX_KEYREF_LOAD_MATERIAL                        ((FUNC_SX_KEYREF_LOAD_MATERIAL)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_KEYREF_LOAD_MATERIAL)))


/** Returns a reference to a key selected by an index.
 *
 * This function initializes a sxkeyref object to use predefined hardware keys.
 * Currently, predefined hardware keys can be used with AES(BA411) and
 * SM4(BA419).
 *
 * The returned object can be passed to any of the sx_aead_create_*() or
 * sx_blkcipher_create_*() functions.
 *
 * @param[in] keyindex index of the hardware key, must be 0 or 1.
 * @return sxkeyref initialized object with configuration of the hardware key
 *         index provided
 */
typedef struct sxkeyref (*FUNC_SX_KEYREF_LOAD_BY_ID)(size_t keyindex);
#define SX_KEYREF_LOAD_BY_ID                           ((FUNC_SX_KEYREF_LOAD_BY_ID)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_KEYREF_LOAD_BY_ID)))
#endif
