/* Copyright (c) 2019-2020 Silex Insight. All Rights reserved. */
#ifndef INTERNAL_HEADER_FILE
#define INTERNAL_HEADER_FILE

#include <stdint.h>
#include <stddef.h>

#ifndef SX_EXTRA_IN_DESCS
#define SX_EXTRA_IN_DESCS 0
#endif
#ifndef SX_HASH_PRIV_SZ
#define SX_HASH_PRIV_SZ 344
#endif

#define SX_BLKCIPHER_PRIV_SZ 16
#define SX_AEAD_PRIV_SZ (70)

struct sx_regs;
struct sx_blkcipher_cmdma_cfg;
struct sx_aead_cmdma_cfg;
struct sxhashalg;

/** A cryptomaster DMA descriptor */
struct sxdesc {
    char *addr;
    struct sxdesc *next;
    uint32_t sz;
    uint32_t dmatag;
};


/** Input and output descriptors and related state for cmdma */
struct sx_dmaslot {
    uint32_t cfg;
    struct sxdesc outdescs[5];
};


/** DMA controller
 *
 * For internal use only. Don't access directly.
 */
struct sx_dmactl {
    struct sx_regs *regs;
    struct sxdesc *d;
    struct sxdesc *out;
    char *mapped;
    struct sx_dmaslot dmamem;
};

/** Key reference
 *
 * Used for making a reference to a key stored in memory or to a key selected
 * by an identifier.
 * Created by SX_KEYREF_LOAD_MATERIAL() or SX_KEYREF_LOAD_BY_ID(). Used by blkcipher and
 * aead creation functions.
 *
 * All members should be considered INTERNAL and may not be accessed directly.
 */
struct sxkeyref {
    const char *key;
    size_t sz;
    uint32_t cfg;
};

/** An AEAD operation
 *
 * To be used with sx_aead_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxaead {
    const struct sx_aead_cmdma_cfg *cfg;
    const char *expectedtag;
    uint8_t tagsz;
    uint8_t headersz;
    uint32_t discardaadsz;
    uint32_t datainsz;
    uint64_t dataintotalsz;
    uint64_t totalaadsz;
    uint8_t granularity;
    int is_in_ctx;
    size_t ctxsz;
    unsigned int compatible;
    const struct sxkeyref *key;
    struct sx_dmactl dma;
    struct sxdesc allindescs[7];
    uint8_t extramem[SX_AEAD_PRIV_SZ];
};


/** A simple block cipher operation
 *
 * To be used with sx_blkcipher_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxblkcipher {
    const struct sx_blkcipher_cmdma_cfg *cfg;
    size_t inminsz;
    size_t granularity;
    uint32_t mode;
    unsigned int compatible;
    const struct sxkeyref *key;
    struct sx_dmactl dma;
    struct sxdesc allindescs[5];
    char extramem[SX_BLKCIPHER_PRIV_SZ];
};


/** A hash operation.
 *
 * To be used with sx_hash_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxhash {
    const struct sxhashalg *algo;
    const struct sx_digesttags *dmatags;
    uint32_t cntindescs;
    size_t totalsz;
    uint32_t feedsz;
    void(*digest)(struct sxhash* c, char *digest);
    struct sx_dmactl dma;
    struct sxdesc allindescs[7 + SX_EXTRA_IN_DESCS];
    uint8_t extramem[SX_HASH_PRIV_SZ];
};


/** A operation to load a countermeasures mask into the hardware.
 *
 * To be used with sx_cm_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxcmmask {
    struct sx_dmactl dma;
    struct sxdesc allindescs[1];
};


/** A simple MAC(message authentication code) operation
 *
 * To be used with sx_mac_*() functions.
 *
 * All members should be considered INTERNAL and may not be accessed
 * directly.
 */
struct sxmac {
    const struct sx_mac_cmdma_cfg *cfg;
    uint32_t cntindescs;
    uint32_t feedsz;
    int macsz;
    unsigned int compatible;
    const struct sxkeyref *key;
    struct sx_dmactl dma;
    struct sxdesc allindescs[5];
    uint8_t extramem[16];
};

#endif
