/** Common simple block cipher modes.
 *
 * All block cipher modes here perform simple encryption and decryption
 * without any authentication.
 *
 * @file
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * encryption and decryption of a message.
   @code
   1. One-shot operation
      a. Encryption
          sx_blkcipher_create_aescbc_enc(ctx, ...)
          SX_BLKCIPHER_CRYPT(ctx, ...)
          SX_BLKCIPHER_RUN(ctx)
          SX_BLKCIPHER_WAIT(ctx)
      b. Decryption
          sx_blkcipher_create_aescbc_dec(ctx, ...)
          SX_BLKCIPHER_CRYPT(ctx, ...)
          SX_BLKCIPHER_RUN(ctx)
          SX_BLKCIPHER_WAIT(ctx)
   2. Context-saving operation
      a. Encryption
          First round:
              sx_blkcipher_create_aescbc_enc(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'first chunk')
              sx_blkcipher_save_state(ctx)
              SX_BLKCIPHER_WAIT(ctx)
          Intermediary rounds:
              sx_blkcipher_resume_state(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'n-th chunk')
              sx_blkcipher_save_state(ctx)
              SX_BLKCIPHER_WAIT(ctx)
          Last round:
              sx_blkcipher_resume_state(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'last chunk')
              SX_BLKCIPHER_RUN(ctx)
              SX_BLKCIPHER_WAIT(ctx)
      b. Decryption
          First round:
              sx_blkcipher_create_aescbc_dec(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'first chunk')
              sx_blkcipher_save_state(ctx)
              SX_BLKCIPHER_WAIT(ctx)
          Intermediary rounds:
              sx_blkcipher_resume_state(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'n-th chunk')
              sx_blkcipher_save_state(ctx)
              SX_BLKCIPHER_WAIT(ctx)
          Last round:
              sx_blkcipher_resume_state(ctx)
              SX_BLKCIPHER_CRYPT(ctx, 'last chunk')
              SX_BLKCIPHER_RUN(ctx)
              SX_BLKCIPHER_WAIT(ctx)
   @endcode
 */

#ifndef BLKCIPHER_API_FILE
#define BLKCIPHER_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/api_table.h"


/** Prepares an AES XTS block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES XTS encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation. expected size
 *                 16, 24 or 32 bytes, must be equal to \p key2 size
 * @param[in] key2 second key used for the block cipher operation, expected
 *                 size 16, 24 or 32 bytes, must be equal to \p key1 size
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using SX_KEYREF_LOAD_MATERIAL().
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESXTS_ENC)(struct sxblkcipher *c, const struct sxkeyref *key1, const struct sxkeyref *key2, const char *iv);
#define SX_BLKCIPHER_CREATE_AESXTS_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESXTS_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESXTS_ENC)))


/** Prepares an AES XTS block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES XTS decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation. expected size
 *                 16, 24 or 32 bytes, must be equal to \p key2 size
 * @param[in] key2 second key used for the block cipher operation, expected
 *                 size 16, 24 or 32 bytes, must be equal to \p key1 size
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using SX_KEYREF_LOAD_MATERIAL().
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESXTS_DEC)(struct sxblkcipher *c, const struct sxkeyref *key1, const struct sxkeyref *key2, const char *iv);
#define SX_BLKCIPHER_CREATE_AESXTS_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESXTS_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESXTS_DEC)))


/** Prepares an AES CTR block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CTR encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCTR_ENC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCTR_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCTR_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCTR_ENC)))


/** Prepares an AES CTR block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CTR decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *            16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCTR_DEC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCTR_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCTR_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCTR_DEC)))


/** Prepares an AES ECB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES ECB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size is
 *                16, 24 or 32 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - AES ECB does not support context saving.
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESECB_ENC)(struct sxblkcipher *c, const struct sxkeyref *key);
#define SX_BLKCIPHER_CREATE_AESECB_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESECB_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESECB_ENC)))


/** Prepares an AES ECB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES ECB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 *
 * @remark - AES ECB does not support context saving.
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESECB_DEC)(struct sxblkcipher *c, const struct sxkeyref *key);
#define SX_BLKCIPHER_CREATE_AESECB_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESECB_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESECB_DEC)))


/** Prepares an AES CBC block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CBC encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
                  16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCBC_ENC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCBC_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCBC_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCBC_ENC)))


/** Prepares an AES CBC block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CBC decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCBC_DEC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCBC_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCBC_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCBC_DEC)))


/** Prepares an AES CFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCFB_ENC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCFB_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCFB_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCFB_ENC)))


/** Prepares an AES CFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESCFB_DEC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESCFB_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESCFB_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESCFB_DEC)))


/** Prepares an AES OFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES OFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESOFB_ENC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESOFB_ENC                 ((FUNC_SX_BLKCIPHER_CREATE_AESOFB_ENC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESOFB_ENC)))


/** Prepares an AES OFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES OFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
  *
 * @pre - key reference provided by \p key must be initialized using
 *        SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID()
 */
typedef int (*FUNC_SX_BLKCIPHER_CREATE_AESOFB_DEC)(struct sxblkcipher *c, const struct sxkeyref *key, const char *iv);
#define SX_BLKCIPHER_CREATE_AESOFB_DEC                 ((FUNC_SX_BLKCIPHER_CREATE_AESOFB_DEC)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CREATE_AESOFB_DEC)))


/** Adds data to be encrypted/decrypted.
 *
 * The function will return immediately.
 *
 * In order to start the operation SX_BLKCIPHER_RUN() must be called.
 *
 * \p sz must ensure the following restrictions based on the mode used:
 *  Algorithm | \p sz        | Remarks
 *  --------: | :----------: | :-----------
 *        ECB | N * 16 bytes | N > 0
 *        CBC | N * 16 bytes | N > 0
 *        CFB | N * 16 bytes | N > 0
 *        OFB | N * 16 bytes | N > 0
 *        XTS | >= 16 bytes  | none
 *        CTR | > 0 bytes    | none
 *
 * The restrictions above are applicable ONLY for the last chunk of the data
 * to be processed when doing a partial operation. \p sz must be a multiple of
 * block size(16 bytes) when doing a partial operation(besides last chunk).
 *
 * @param[inout] c block cipher operation context
 * @param[in] datain data to be encrypted or decrypted, with size \p sz
 * @param[in] sz size, in bytes, of data to be decrypted
 * @param[out] dataout encrypted or decrypted data, must have \p sz bytes
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_blkcipher_create_*() functions must be called first
 */
typedef int (*FUNC_SX_BLKCIPHER_CRYPT)(struct sxblkcipher *c, const char *datain, size_t sz, char *dataout);
#define SX_BLKCIPHER_CRYPT                             ((FUNC_SX_BLKCIPHER_CRYPT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_CRYPT)))


/** Starts a block cipher operation.
 *
 * This function is used to start an encryption or a decryption based on what
 * create function was used, sx_blkcipher_create_*_enc() or
 * sx_blkcipher_create_*_dec(). The function will return immediately.
 *
 * The result will be transfered only after the operation is successfully
 * completed. The user shall check operation status with sx_blkcipher_status()
 * or SX_BLKCIPHER_WAIT().
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_TOO_SMALL
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - SX_BLKCIPHER_CRYPT() function must be called first
 */
typedef int (*FUNC_SX_BLKCIPHER_RUN)(struct sxblkcipher *c);
#define SX_BLKCIPHER_RUN                               ((FUNC_SX_BLKCIPHER_RUN)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_RUN)))


/** Resumes AES operation in context-saving.
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_blkcipher_save_state() in the sxblkcipher
 * operation context \p c. It must be called with the same sxblkcipher operation
 * context \p c that was used with sx_sxblkcipher_save_state(). It will reserve
 * all hardware resources required to run the partial AES operation.
 * Previously used mode and direction are already stored in sxblkcipher \p c.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions, except the sx_blkcipher_create_*()
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
 * @pre - sx_blkcipher_create_aes*() and sx_blkcipher_save_state() functions
 *        must be called before, for first part of the message.
 * @pre - must be called for each part of the message(besides first), before
 *        sx_blkchiper_crypt() or sx_blkcipher_save_state().
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 * @remark - AES ECB does not support context saving.
 */
typedef int (*FUNC_SX_BLKCIPHER_RESUME_STATE)(struct sxblkcipher *c);
#define SX_BLKCIPHER_RESUME_STATE                      ((FUNC_SX_BLKCIPHER_RESUME_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_RESUME_STATE)))


/** Starts a partial block cipher operation.
 *
 * This function is used to start a partial encryption or decryption of
 * \p datain. The function will return immediately.
 *
 * The partial result will be transfered to \p dataout only after the operation
 * is successfully completed. The user shall check operation status with
 * sx_blkcipher_status() or SX_BLKCIPHER_WAIT().
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 *
 * @pre - SX_BLKCIPHER_CRYPT() should be called first.
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
typedef int (*FUNC_SX_BLKCIPHER_SAVE_STATE)(struct sxblkcipher *c);
#define SX_BLKCIPHER_SAVE_STATE                        ((FUNC_SX_BLKCIPHER_SAVE_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_SAVE_STATE)))


/** Waits until the given block cipher operation has finished
 *
 * This function returns when the block cipher operation was successfully
 * completed, or when an error has occurred that caused the operation to
 * terminate. The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_blkcipher_create_*() is used.
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 *
 * @see sx_blkcipher_status().
 *
 * @remark - this function is blocking until operation finishes.
 */
typedef int (*FUNC_SX_BLKCIPHER_WAIT)(struct sxblkcipher *c);
#define SX_BLKCIPHER_WAIT                              ((FUNC_SX_BLKCIPHER_WAIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_WAIT)))


/** Returns the block cipher operation status.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the block cipher operation has ended and all resources used by block cipher
 * operation context \p c have been released. In this case, \p c cannot be used
 * for a new operation until one of the sx_blkcipher_create_*() functions is
 * called again.
 *
 * @param[inout] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - SX_BLKCIPHER_CRYPT and SX_BLKCIPHER_RUN() functions must be called
 *        first
 */
typedef int (*FUNC_SX_BLKCIPHER_STATUS)(struct sxblkcipher *c);
#define SX_BLKCIPHER_STATUS                            ((FUNC_SX_BLKCIPHER_STATUS)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_BLKCIPHER_STATUS)))
#endif
