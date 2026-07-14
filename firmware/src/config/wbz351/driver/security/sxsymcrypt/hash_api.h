/** Cryptographic message hashing.
 *
 * @file
 *
 * @copyright Copyright (c) 2019-2020 Silex Insight. All Rights reserved.
 *
 * Examples:
 * The following examples show typical sequences of function calls for hashing
 * a message.
   @code
   1. Single-call hash computation
       SX_HASH_CREATE(ctx, &sxhashalg_sha2_256, ctxsize)
       SX_HASH_FEED(ctx, 'chunk 1')
       SX_HASH_FEED(ctx, 'chunk 2')
       SX_HASH_DIGEST(ctx)
       SX_HASH_WAIT(ctx)
   2. Context-saving hash computation
       First round:
           SX_HASH_CREATE(ctx, &sxhashalg_sha2_256, ctxsize)
           SX_HASH_FEED(ctx, 'chunk 1 of first round')
           SX_HASH_FEED(ctx, 'chunk 2 of first round')
           SX_HASH_SAVE_STATE(ctx)
           SX_HASH_WAIT(ctx)
       Intermediary rounds:
           SX_HASH_RESUME_STATE(ctx)
           SX_HASH_FEED(ctx, 'chunk 1 of round i')
           SX_HASH_FEED(ctx, 'chunk 2 of round i')
           SX_HASH_FEED(ctx, 'chunk 3 of round i')
           SX_HASH_SAVE_STATE(ctx)
           SX_HASH_WAIT(ctx)
       Last round:
           SX_HASH_RESUME_STATE(ctx)
           SX_HASH_FEED(ctx, 'chunk 1 of last round')
           SX_HASH_FEED(ctx, 'chunk 2 of last round')
           SX_HASH_DIGEST(ctx)
           SX_HASH_WAIT(ctx)
   @endcode
 */

#ifndef HASH_API_FILE
#define HASH_API_FILE

#include <stddef.h>
#include "driver/security/sxsymcrypt/internal.h"
#include "driver/security/sxsymcrypt/sha2_api.h"
//#include "driver/security/sxsymcrypt/sha3.h"
#include "driver/security/sxsymcrypt/sha1_api.h"
#include "driver/security/sxsymcrypt/sm3_api.h"
#include "driver/security/api_table.h"

//struct sxhash;


/** Creates a hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] alg hash algorithm
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 */
typedef int (*FUNC_SX_HASH_CREATE)(struct sxhash *c, const struct sxhashalg *alg, size_t csz);
#define SX_HASH_CREATE                                 ((FUNC_SX_HASH_CREATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_CREATE)))


/** Return the digest size produced by the given hash algorithm */
typedef size_t (*FUNC_SX_HASH_GET_ALG_DIGESTSZ)(const struct sxhashalg *alg);
#define SX_HASH_GET_ALG_DIGESTSZ                       ((FUNC_SX_HASH_GET_ALG_DIGESTSZ)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_GET_ALG_DIGESTSZ)))


/** Return the block size used by the given hash algorithm */
typedef size_t (*FUNC_SX_HASH_GET_ALG_BLOCKSZ)(const struct sxhashalg *alg);
#define SX_HASH_GET_ALG_BLOCKSZ                        ((FUNC_SX_HASH_GET_ALG_BLOCKSZ)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_GET_ALG_BLOCKSZ)))


/** Resume hashing in context-saving (partial hashing).
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by SX_HASH_SAVE_STATE() in the sxhash operation
 * context \p c. It must be called with the same sxhash operation context
 * \p c that was used with SX_HASH_SAVE_STATE(). It will reserve all
 * hardware resources required to run the partial hashing.
 *
 * @param[inout] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 *
 * @pre - SX_HASH_SAVE_STATE() function must be called before
 * @pre - must be called before SX_HASH_FEED() for the next partial message
 */
typedef int (*FUNC_SX_HASH_RESUME_STATE)(struct sxhash *c);
#define SX_HASH_RESUME_STATE                           ((FUNC_SX_HASH_RESUME_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_RESUME_STATE)))

/** Assign data to be hashed.
 *
 * This function adds a chunk of data to be hashed. It can be called multiple
 * times to assemble pieces of the message scattered in memory.
 *
 * In context-saving, the sum of the sizes of the chunks fed must be multiple
 * of the block size of the algorithm used. If this condition is not met before
 * calling SX_HASH_SAVE_STATE(), SX_HASH_SAVE_STATE() will return
 * ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @param[inout] c hash operation context
 * @param[in] msg message to be hashed
 * @param[in] sz size in bytes of \p msg, maximum value is 2^24-1 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_FEED_COUNT_EXCEEDED
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the SX_HASH_CREATE_*() functions must be called first
 * @pre - if in context-saving, SX_HASH_RESUME_STATE() must be called first
 *
 * @remark - if return value is ::SX_ERR_FEED_COUNT_EXCEEDED or
 *           ::SX_ERR_TOO_BIG, \p c cannot be used anymore.
 * @remark - if this function is called with \p sz equal to 0, no data will be
 *           assigned to be hashed, ::SX_OK will be returned.
 * @remark - default maximum number of feeds for single-call digest is 6 and for
 *           context-saving is 4.
 * @remark - maximum sum of the chunk sizes fed is 2^32-1 bytes
 */
typedef int (*FUNC_SX_HASH_FEED)(struct sxhash *c, const char *chunk, size_t sz);
#define SX_HASH_FEED                                   ((FUNC_SX_HASH_FEED)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_FEED)))

/** Starts the partial hashing operation.
 *
 * This function updates the partial hashing based on the data chunks fed since
 * the last call to SX_HASH_RESUME_STATE().
 *
 * In order to export the state of partial hashing, the total size of data
 * fed in the current resume-save step must be multiple of block size of
 * the algorithm used. For SHA1/224/256/SM3 block size is 64 bytes and
 * for SHA384/512 block size is 128 bytes.
 *
 * The function will return immediately. The hash state will be saved
 * in the sxhash structure after the operation successfully completed.
 * The user shall check operation status with SX_HASH_STATUS() or
 * SX_HASH_WAIT().
 *
 * @param[inout] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - one of the SX_HASH_CREATE_*() functions or SX_HASH_RESUME_STATE()
 *        must be called first
 *
 * @remark - if return value is ::SX_ERR_WRONG_SIZE_GRANULARITY, \p c cannot
 *           be used anymore.
 * @remark - the content of the input data buffers, provided with previous
 *           calls to SX_HASH_FEED(), should not be changed until the operation
 *           is completed. Checking the completion of an operation is done by
 *           using SX_HASH_WAIT() or SX_HASH_STATUS().
 */
typedef int (*FUNC_SX_HASH_SAVE_STATE)(struct sxhash *c);
#define SX_HASH_SAVE_STATE                             ((FUNC_SX_HASH_SAVE_STATE)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_SAVE_STATE)))

/** Starts the hashing operation.
 *
 * This function is used to start the computation of the digest.
 * If used in context-saving approach, this function will compute the digest
 * based on last computed state and last chunks of the message.
 *
 * The function will return immediately. The result will be transfered to
 * \p digest only after the operation is successfully completed. The user shall
 * check operation status with SX_HASH_STATUS() or SX_HASH_WAIT().
 *
 * @param[inout] c hash operation context
 * @param[out] digest result of the hash operation, user must allocate enough
 *                    memory for it. In order to get the amount of memory needed
 *                    for the digest, the user can use SX_HASH_GET_DIGESTSZ() or
 *                    check the corresponding SX_HASH_CREATE_*() function.
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 *
 * @pre - one of the SX_HASH_CREATE_*() functions must be called first
 *
 * @remark - the content of the input data buffers, provided with previous
 *           calls to SX_HASH_FEED(), should not be changed until the operation
 *           is completed. Checking the completion of an operation is done by
 *           using SX_HASH_WAIT() or SX_HASH_STATUS().
 */
typedef int (* FUNC_SX_HASH_DIGEST)(struct sxhash *c, char *digest);
#define SX_HASH_DIGEST                                 ((FUNC_SX_HASH_DIGEST)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_DIGEST)))

/** Waits until the given hash operation has finished
 *
 * This function returns when the hash operation was successfully completed,
 * or when an error has occurred that caused the operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless SX_HASH_CREATE_*() is used.
 *
 * @param[inout] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - SX_HASH_DIGEST() functions must be called first
 * @pre - if in context-saving, SX_HASH_SAVE_STATE() must be called first
 *
 * @see SX_HASH_STATUS().
 *
 * @remark - this function is blocking until operation finishes.
 * @remark - this function calls SX_HASH_STATUS() in loop until operation
 *           is completed.
 */
typedef int (*FUNC_SX_HASH_WAIT)(struct sxhash *c);
#define SX_HASH_WAIT                                   ((FUNC_SX_HASH_WAIT)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_WAIT)))

/** Returns the status of the given hash operation context.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the hash operation has ended and all associated hardware resources
 * used by hash operation context \p c have been released.
 * If SX_HASH_DIGEST() was used, \p c cannot be used for a new operation
 * until one of the SX_HASH_CREATE_*() functions is called again.
 * If SX_HASH_SAVE_STATE() was used, then \p c shall be reused with
 * sx_hash_resume().
 *
 * @param[inout] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - SX_HASH_DIGEST() functions must be called first
 * @pre - if in context-saving, SX_HASH_SAVE_STATE() must be called first.
 */
typedef int (*FUNC_SX_HASH_STATUS)(struct sxhash *c);
#define SX_HASH_STATUS                                 ((FUNC_SX_HASH_STATUS)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_STATUS)))

/** Returns digest size in bytes for the hash operation context.
 *
 * @param[in] c hash operation context
 * @return digest size in bytes of the algorithm specified by \p c
 *
 * @pre - one of the SX_HASH_CREATE_*() functions must be called first
 */
typedef size_t (*FUNC_SX_HASH_GET_DIGESTSZ)(struct sxhash *c);
#define SX_HASH_GET_DIGESTSZ                           ((FUNC_SX_HASH_GET_DIGESTSZ)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_GET_DIGESTSZ)))

/** Returns block size in bytes for the hash operation context.
 *
 * @param[in] c hash operation context
 * @return block size in bytes of the algorithm specified by \p c
 *
 * @pre - one of the SX_HASH_CREATE_*() functions must be called first
 */
typedef size_t (*FUNC_SX_HASH_GET_BLOCKSZ)(struct sxhash *c);
#define SX_HASH_GET_BLOCKSZ                            ((FUNC_SX_HASH_GET_BLOCKSZ)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_GET_BLOCKSZ)))


/** Abandon a created hash operation context.
 *
 * A created hash operation context can have reserved hardware and
 * software resources. Those are free-ed automatically on error
 * or when the operation finishes as reported by SX_HASH_STATUS() or
 * SX_HASH_WAIT(). If for some reason, the hash operation will not
 * be started, it can be abandoned which will release all reserved
 * resources.
 *
 * Can only be called after a call to one of the SX_HASH_CREATE_*() or
 * after sx_hash_resume(). It must be called *BEFORE* starting to run
 * the operation with SX_HASH_DIGEST() or SX_HASH_SAVE_STATE().
 *
 * @param[inout] c hash operation context
 */
typedef void (*FUNC_SX_HASH_ABANDON)(struct sxhash *c);
#define SX_HASH_ABANDON                                ((FUNC_SX_HASH_ABANDON)(*(uint32_t *)(API_TABLE_BASE_ADDRESS + ATO_SX_HASH_ABANDON)))

#endif
