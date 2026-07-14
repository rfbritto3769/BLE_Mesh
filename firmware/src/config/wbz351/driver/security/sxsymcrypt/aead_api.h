/** Authenticated encryption with associated data(AEAD).
 *
 * @file
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * encryption and decryption a message.
   @code
   1. One-shot operation
      a. Encryption
         SX_AEAD_CREATE_AESGCM_ENC(ctx, ...)
         SX_AEAD_FEED_AAD(ctx, aad, aadsz)
         SX_AEAD_CRYPT(ctx, datain, datainz, dataout)
         SX_AEAD_PRODUCE_TAG(ctx, tag)
         SX_AEAD_WAIT(ctx)
      b. Decryption
         SX_AEAD_CREATE_AESGCM_DEC(ctx, ...)
         SX_AEAD_FEED_AAD(ctx, aad, aadsz)
         SX_AEAD_CRYPT(ctx, datain, datainz, dataout)
         SX_AEAD_VERIFY_TAG(ctx, tag)
         SX_AEAD_WAIT(ctx)
   2. Context-saving operation
         a. Encryption
         First round:
            SX_AEAD_CREATE_AESGCM_ENC(ctx)
            SX_AEAD_FEED_AAD(ctx, aad, aadsz)
            SX_AEAD_CRYPT(ctx, 'first chunk')
            SX_AEAD_SAVE_STATE(ctx)
            SX_AEAD_WAIT(ctx)
         Intermediary rounds:
            SX_AEAD_RESUME_STATE(ctx)
            SX_AEAD_CRYPT(ctx, 'n-th chunk')
            SX_AEAD_SAVE_STATE(ctx)
            SX_AEAD_WAIT(ctx)
         Last round:
            SX_AEAD_RESUME_STATE(ctx)
            SX_AEAD_CRYPT(ctx, 'last chunk')
            SX_AEAD_PRODUCE_TAG(ctx, tag)
            SX_AEAD_WAIT(ctx)
         b. Decryption
         First round:
            SX_AEAD_CREATE_AESGCM_DEC(ctx)
            SX_AEAD_FEED_AAD(ctx, aad, aadsz)
            SX_AEAD_CRYPT(ctx, 'first chunk')
            SX_AEAD_SAVE_STATE(ctx)
            SX_AEAD_WAIT(ctx)
         Intermediary rounds:
            SX_AEAD_RESUME_STATE(ctx)
            SX_AEAD_CRYPT(ctx, 'n-th chunk')
            SX_AEAD_SAVE_STATE(ctx)
            SX_AEAD_WAIT(ctx)
         Last round:
            SX_AEAD_RESUME_STATE(ctx)
            SX_AEAD_CRYPT(ctx, 'last chunk')
            SX_AEAD_VERIFY_TAG(ctx, tag)
            SX_AEAD_WAIT(ctx)
   @endcode
 */

#ifndef AEAD_API_FILE
#define AEAD_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"

/** Initialization vector (IV) size, in bytes, for GCM encryption/decryption */
#define SX_GCM_IV_SZ 12u

/** Size, in bytes, of GCM authentication tag */
#define SX_GCM_TAG_SZ 16u

/** Maximum size, in bytes, of CCM authentication tag */
#define SX_CCM_MAX_TAG_SZ 16u


/** Prepares an AES GCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - \p key and \p iv buffers should not be changed until the operation
 *           is completed.
 * @remark - GMAC is supported by using GCM with plaintext with size 0.
 * @remark - GCM and GMAC support AAD split in multiple chunks, using context
 *           saving.
 */
typedef int (*FUNC_SX_AEAD_CREATE_AESGCM_ENC)(struct sxaead *c, const struct sxkeyref *key, const char *iv);
#define SX_AEAD_CREATE_AESGCM_ENC         ((FUNC_SX_AEAD_CREATE_AESGCM_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CREATE_AESGCM_ENC)))


/** Prepares an AES GCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - \p key and \p iv buffers should not be changed until the operation
 *           is completed.
 * @remark - GMAC is supported by using GCM with ciphertext with size 0.
 * @remark - GCM and GMAC support AAD split in multiple chunks, using context
 *           saving.
 */
typedef int (*FUNC_SX_AEAD_CREATE_AESGCM_DEC)(struct sxaead *c, const struct sxkeyref *key, const char *iv);
#define SX_AEAD_CREATE_AESGCM_DEC                      ((FUNC_SX_AEAD_CREATE_AESGCM_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CREATE_AESGCM_DEC)))


/** Prepares an AES CCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] noncesz size, in bytes, of the nonce, between 7 and 13 bytes
 * @param[in] nonce nonce used for the AEAD operation, with size \p noncesz
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD)
 * @param[in] datasz size, in bytes, of the data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - the same aadsz and datasz must be provided to sx_aead_encrypt()
 *           or sx_aead_decrypt() functions.
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 * @remark - CCM DOES NOT support AAD split in multiple chunks
 */
typedef int (*FUNC_SX_AEAD_CREATE_AESCCM_ENC)(struct sxaead *c, const struct sxkeyref *key, const char *nonce, size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);
#define SX_AEAD_CREATE_AESCCM_ENC                      ((FUNC_SX_AEAD_CREATE_AESCCM_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CREATE_AESCCM_ENC)))


/** Prepares an AES CCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] noncesz size, in bytes, of the nonce, between 7 and 13 bytes
 * @param[in] nonce nonce used for the AEAD operation, with size \p noncesz
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD)
 * @param[in] datasz size, in bytes, of the data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - the same aadsz and datasz must be provided to sx_aead_encrypt()
 *           or sx_aead_decrypt() functions.
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 * @remark - CCM DOES NOT support AAD split in multiple chunks
 */
typedef int (*FUNC_SX_AEAD_CREATE_AESCCM_DEC)(struct sxaead *c, const struct sxkeyref *key, const char *nonce, size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);
#define SX_AEAD_CREATE_AESCCM_DEC                      ((FUNC_SX_AEAD_CREATE_AESCCM_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CREATE_AESCCM_DEC)))


/** Adds AAD chunks
 *
 * This function is used for adding AAD buffer given by \p aad. The function
 * will return immediately.
 *
 * @param[inout] c AEAD operation context
 * @param[in] aad additional authentication data(AAD), with size \p aadsz
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD),
 *                  can be 0 if \p aad is empty
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_create_*() functions must be called first
 *
 * @remark - the additional authentication data can be empty(\p aadsz = 0)
 * @remark - \p aad buffer should not be changed until the operation is
 *           completed.
 */
typedef int (*FUNC_SX_AEAD_FEED_AAD)(struct sxaead *c, const char *aad, size_t aadsz);
#define SX_AEAD_FEED_AAD                               ((FUNC_SX_AEAD_FEED_AAD)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_FEED_AAD)))


/** Adds data to be encrypted or decrypted.
 *
 * This function is used for adding data to be processed. The function will
 * return immediately.
 *
 * The result of the operation will be transfered to \p dataout after the
 * operation is successfully completed.
 *
 * For context saving, \p datain size(\p datainsz) must be a multiple of 16
 * bytes for AES GCM and CCM and a multiple of 64 bytes for ChaCha20Poly1305,
 * except the last buffer.
 *
 * @param[inout] c AEAD operation context
 * @param[in] datain data to be encrypted or decryoted, with size \p datainsz
 * @param[in] datainsz size, in bytes, of the data to be encrypted or decrypted
 * @param[out] dataout encrypted or decrypted data, must have \p datainsz bytes
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_create_*() functions must be called first
 *
 * @remark - \p datain buffer should not be changed until the operation is
 *           completed.
 */
typedef int (*FUNC_SX_AEAD_CRYPT)(struct sxaead *c, const char *datain, size_t datainsz, char *dataout);
#define SX_AEAD_CRYPT                                  ((FUNC_SX_AEAD_CRYPT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_CRYPT)))


/** Starts an AEAD encryption and tag computation.
 *
 * The function will return immediately.
 *
 * The computed tag will be transfered to \p tag only after the operation is
 * successfully completed.
 *
 * The user shall check operation status with sx_aead_status() or SX_AEAD_WAIT().
 *
 * @param[inout] c AEAD operation context
 * @param[out] tag authentication tag
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the SX_AEAD_FEED_AAD() or SX_AEAD_CRYPT() functions must be
 *        called first
 *
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
typedef int (*FUNC_SX_AEAD_PRODUCE_TAG)(struct sxaead *c, char *tagout);
#define SX_AEAD_PRODUCE_TAG                            ((FUNC_SX_AEAD_PRODUCE_TAG)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_PRODUCE_TAG)))


/** Starts an AEAD decryption and tag validation.
 *
 * The function will return immediately.
 *
 * The user shall check operation status with sx_aead_status() or SX_AEAD_WAIT().
 *
 * @param[inout] c AEAD operation context
 * @param[in] tag authentication tag
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the SX_AEAD_FEED_AAD() or SX_AEAD_CRYPT() functions must be
 *        called first
 *
 * @remark - \p tag buffer should not be changed until the operation is
 *           completed.
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
typedef int (*FUNC_SX_AEAD_VERIFY_TAG)(struct sxaead *c, const char *tagin);
#define SX_AEAD_VERIFY_TAG                             ((FUNC_SX_AEAD_VERIFY_TAG)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_VERIFY_TAG)))


/** Resumes AEAD operation in context-saving.
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_aead_save_state() in the sxaead
 * operation context \p c. It must be called with the same sxaead operation
 * context \p c that was used with sx_aead_save_state(). It will reserve
 * all hardware resources required to run the partial AEAD operation.
 * Previously used mode and direction are already stored in sxaead \p c.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions, except the sx_aead_create_*()
 * functions.
 *
 * @param[inout] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - sx_aead_create_aes*() and sx_aead_save_state() functions
 *        must be called before, for first part of the message.
 * @pre - must be called for each part of the message(besides first), before
 *        SX_AEAD_CRYPT() or sx_aead_save_state().
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
typedef int (*FUNC_SX_AEAD_RESUME_STATE)(struct sxaead *c);
#define SX_AEAD_RESUME_STATE                           ((FUNC_SX_AEAD_RESUME_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_RESUME_STATE)))


/** Starts a partial AEAD operation.
 *
 * This function is used to start a partial encryption or decryption of
 * \p datain. The function will return immediately.
 *
 * The partial result will be transfered to \p dataout only after the operation
 * is successfully completed. The user shall check operation status with
 * sx_aead_status() or SX_AEAD_WAIT().
 *
 * @param[inout] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - SX_AEAD_CRYPT() should be called first.
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 * @remark - when in context saving, the sizes of the chunks fed must be
 *           multiple of 16 bytes, besides the last chunk that can be any size,
 *           but not 0
 */
typedef int (*FUNC_SX_AEAD_SAVE_STATE)(struct sxaead *c);
#define SX_AEAD_SAVE_STATE                             ((FUNC_SX_AEAD_SAVE_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_SAVE_STATE)))


/** Waits until the given AEAD operation has finished
 *
 * This function returns when the AEAD operation was successfully completed,
 * or when an error has occurred that caused the operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_aead_create_*() is used.
 *
 * making \p c unusable for a new operation without calling, first, one of the
 * sx_aead_create_*() functions.
 *
 * @param[inout] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - sx_aead_encrypt or sx_aead_decrypt() function must be called first
 *
 * @see sx_aead_status().
 *
 * @remark - this function is blocking until operation finishes.
 */
typedef int (*FUNC_SX_AEAD_WAIT)(struct sxaead *c);
#define SX_AEAD_WAIT                                   ((FUNC_SX_AEAD_WAIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_WAIT)))


/** Returns the AEAD operation status.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the AEAD operation has ended and all resources used by the AEAD operation
 * context \p c have been released. In this case, \p c cannot be used for a new
 * operation until one of the sx_aead_create_*() functions is called again.
 *
 * @param[inout] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - sx_aead_encrypt or sx_aead_decrypt() function must be called first
 *
 * @remark -  if authentication fails during decryption, ::SX_ERR_INVALID_TAG
 *            will be returned. In this case, the decrypted text is not valid
 *            and shall not be used.
 */
typedef int (*FUNC_SX_AEAD_STATUS)(struct sxaead *c);
#define SX_AEAD_STATUS                                 ((FUNC_SX_AEAD_STATUS)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_AEAD_STATUS)))
#endif
