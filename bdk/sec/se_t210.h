/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2021 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SE_T210_H
#define _SE_T210_H

#include <utils/types.h>

#define SE_CRYPTO_QUEUE_LENGTH 50
#define SE_MAX_SRC_SG_COUNT    50
#define SE_MAX_DST_SG_COUNT    50

#define SE_AES_KEYSLOT_COUNT   16
#define SE_RSA_KEYSLOT_COUNT   2
#define SE_MAX_LAST_BLOCK_SIZE 0xFFFFF

#define SE_AES_BLOCK_SIZE   16
#define SE_AES_IV_SIZE      16
#define SE_AES_MIN_KEY_SIZE 16
#define SE_AES_MAX_KEY_SIZE 32
#define SE_KEY_128_SIZE     16
#define SE_KEY_192_SIZE     24
#define SE_KEY_256_SIZE     32
#define SE_SHA_192_SIZE     24
#define SE_SHA_256_SIZE     32
#define SE_SHA_384_SIZE     48
#define SE_SHA_512_SIZE     64
#define SE_RNG_IV_SIZE		16
#define SE_RNG_DT_SIZE		16
#define SE_RNG_KEY_SIZE		16
#define SE_RNG_SEED_SIZE (SE_RNG_IV_SIZE + SE_RNG_KEY_SIZE + SE_RNG_DT_SIZE)

#define SE_AES_CMAC_DIGEST_SIZE 16
#define SE_RSA512_DIGEST_SIZE   64
#define SE_RSA1024_DIGEST_SIZE  128
#define SE_RSA1536_DIGEST_SIZE  192
#define SE_RSA2048_DIGEST_SIZE  256

#define  DECRYPT   0
#define  ENCRYPT   1

/* SE register definitions */
#define SE_SE_SECURITY_REG 0x000
#define  SE_HARD_SETTING   BIT(0)
#define  SE_ENG_DIS        BIT(1)
#define  SE_PERKEY_SETTING BIT(2)
#define  SE_SOFT_SETTING   BIT(16)

#define SE_TZRAM_SECURITY_REG 0x004
#define  SE_TZRAM_HARD_SETTING BIT(0)
#define  SE_TZRAM_ENG_DIS      BIT(1)

#define SE_OPERATION_REG 0x008
#define  SE_OP_ABORT       0
#define  SE_OP_START       1
#define  SE_OP_RESTART_OUT 2
#define  SE_OP_CTX_SAVE    3
#define  SE_OP_RESTART_IN  4

#define SE_INT_ENABLE_REG	0x00C
#define SE_INT_STATUS_REG	0x010
#define  SE_INT_IN_LL_BUF_RD  BIT(0)
#define  SE_INT_IN_DONE       BIT(1)
#define  SE_INT_OUT_LL_BUF_WR BIT(2)
#define  SE_INT_OUT_DONE      BIT(3)
#define  SE_INT_OP_DONE       BIT(4)
#define  SE_INT_RESEED_NEEDED BIT(5)
#define  SE_INT_ERR_STAT      BIT(16)

#define SE_CONFIG_REG 0x014
#define  DST_MEMORY      0
#define  DST_HASHREG     1
#define  DST_KEYTABLE    2
#define  DST_SRK         3
#define  DST_RSAREG      4
#define  SE_CONFIG_DST(x)      ((x) << 2)
#define  ALG_NOP         0
#define  ALG_AES_DEC     1
#define  SE_CONFIG_DEC_ALG(x)  ((x) << 8)
#define  ALG_NOP         0
#define  ALG_AES_ENC     1
#define  ALG_RNG         2
#define  ALG_SHA         3
#define  ALG_RSA         4
#define  SE_CONFIG_ENC_ALG(x)  ((x) << 12)
#define  MODE_KEY128     0
#define  MODE_KEY192     1
#define  MODE_KEY256     2
#define  MODE_SHA1       0
#define  MODE_SHA224     4
#define  MODE_SHA256     5
#define  MODE_SHA384     6
#define  MODE_SHA512     7
#define  SE_CONFIG_DEC_MODE(x) ((x) << 16)
#define  SE_CONFIG_ENC_MODE(x) ((x) << 24)

#define SE_IN_LL_ADDR_REG        0x018
#define SE_IN_CUR_BYTE_ADDR_REG  0x01C
#define SE_IN_CUR_LL_ID_REG      0x020
#define SE_OUT_LL_ADDR_REG       0x024
#define SE_OUT_CUR_BYTE_ADDR_REG 0x028
#define SE_OUT_CUR_LL_ID_REG     0x02C

#define SE_HASH_RESULT_REG 0x030
#define  SE_HASH_RESULT_REG_COUNT 16

#define SE_CONTEXT_SAVE_CONFIG_REG 0x070
#define  KEYS_0_3        0
#define  KEYS_4_7        1
#define  ORIGINAL_IV     2
#define  UPDATED_IV      3
#define  SE_CONTEXT_AES_WORD_QUAD(x)    ((x) << 0)
#define  SE_CONTEXT_AES_KEY_INDEX(x)    ((x) << 8)
#define  KEYS_0_3        0
#define  KEYS_4_7        1
#define  KEYS_8_11       2
#define  KEYS_12_15      3
#define  SE_CONTEXT_RSA_WORD_QUAD(x)    ((x) << 12)
#define  SLOT0_EXPONENT  0
#define  SLOT0_MODULUS   1
#define  SLOT1_EXPONENT  2
#define  SLOT1_MODULUS   3
#define  SE_CONTEXT_RSA_KEY_INDEX(x)    ((x) << 16)
#define  STICKY_0_3      0
#define  STICKY_4_7      1
#define  SE_CONTEXT_STICKY_WORD_QUAD(x) ((x) << 24)
#define  STICKY_BITS     0
#define  RSA_KEYTABLE    1
#define  AES_KEYTABLE    2
#define  MEM             4
#define  SRK             6
#define SE_CONTEXT_SRC(x)               ((x) << 29)

#define SE_CTX_SAVE_AUTO_T210B01_REG 0x074
#define  SE_CTX_SAVE_AUTO_ENABLE		BIT(0)
#define  SE_CTX_SAVE_AUTO_LOCK          BIT(8)
#define  SE_CTX_SAVE_AUTO_CURR_CNT_MASK (0x3FF << 16)

#define SE_CRYPTO_LAST_BLOCK 0x080

#define SE_SHA_CONFIG_REG 0x200
#define  SHA_CONTINUE  0
#define  SHA_INIT_HASH 1

#define SE_SHA_MSG_LENGTH_0_REG 0x204
#define SE_SHA_MSG_LENGTH_1_REG 0x208
#define SE_SHA_MSG_LENGTH_2_REG 0x20C
#define SE_SHA_MSG_LENGTH_3_REG 0x210
#define SE_SHA_MSG_LEFT_0_REG   0x214
#define SE_SHA_MSG_LEFT_1_REG   0x218
#define SE_SHA_MSG_LEFT_2_REG   0x21C
#define SE_SHA_MSG_LEFT_3_REG   0x220

#define SE_CRYPTO_SECURITY_PERKEY_REG 0x280
#define  SE_KEY_LOCK_FLAG   0x80
#define SE_CRYPTO_KEYTABLE_ACCESS_REG 0x284
#define  SE_CRYPTO_KEYTABLE_ACCESS_REG_COUNT 16
#define  SE_KEY_TBL_DIS_KEYREAD_FLAG    BIT(0)
#define  SE_KEY_TBL_DIS_KEYUPDATE_FLAG  BIT(1)
#define  SE_KEY_TBL_DIS_OIVREAD_FLAG    BIT(2)
#define  SE_KEY_TBL_DIS_OIVUPDATE_FLAG  BIT(3)
#define  SE_KEY_TBL_DIS_UIVREAD_FLAG    BIT(4)
#define  SE_KEY_TBL_DIS_UIVUPDATE_FLAG  BIT(5)
#define  SE_KEY_TBL_DIS_KEYUSE_FLAG     BIT(6)
#define  SE_KEY_TBL_DIS_KEY_ACCESS_FLAG 0x7F

#define SE_CRYPTO_CONFIG_REG 0x304
#define  HASH_DISABLE   0
#define  HASH_ENABLE    1
#define  SE_CRYPTO_HASH(x)          ((x) << 0)
#define  XOR_BYPASS     0
#define  XOR_TOP        2
#define  XOR_BOTTOM     3
#define  SE_CRYPTO_XOR_POS(x)       ((x) << 1)
#define  INPUT_MEMORY   0
#define  INPUT_RANDOM   1
#define  INPUT_AESOUT   2
#define  INPUT_LNR_CTR  3
#define  SE_CRYPTO_INPUT_SEL(x)     ((x) << 3)
#define  VCTRAM_MEM     0
#define  VCTRAM_AESOUT  2
#define  VCTRAM_PREVMEM 3
#define  SE_CRYPTO_VCTRAM_SEL(x)    ((x) << 5)
#define  IV_ORIGINAL    0
#define  IV_UPDATED     1
#define  SE_CRYPTO_IV_SEL(x)        ((x) << 7)
#define  CORE_DECRYPT   0
#define  CORE_ENCRYPT   1
#define  SE_CRYPTO_CORE_SEL(x)      ((x) << 8)
#define  SE_CRYPTO_KEYSCH_BYPASS BIT(10)
#define  SE_CRYPTO_CTR_CNTN(x)      ((x) << 11)
#define  SE_CRYPTO_KEY_INDEX(x)     ((x) << 24)
#define  MEMIF_AHB      0
#define  MEMIF_MCCIF    1
#define  SE_CRYPTO_MEMIF(x)         ((x) << 31)

#define SE_CRYPTO_LINEAR_CTR_REG 0x308
#define  SE_CRYPTO_LINEAR_CTR_REG_COUNT 4

#define SE_CRYPTO_BLOCK_COUNT_REG 0x318

#define SE_CRYPTO_KEYTABLE_ADDR_REG 0x31C
#define  SE_KEYTABLE_PKT(x)         ((x) << 0)
#define  KEYS_0_3        0
#define  KEYS_4_7        1
#define  ORIGINAL_IV     2
#define  UPDATED_IV      3
#define  SE_KEYTABLE_QUAD(x)        ((x) << 2)
#define  SE_KEYTABLE_SLOT(x)        ((x) << 4)

#define SE_CRYPTO_KEYTABLE_DATA_REG 0x320

#define SE_CRYPTO_KEYTABLE_DST_REG 0x330
#define  KEYS_0_3        0
#define  KEYS_4_7        1
#define  ORIGINAL_IV     2
#define  UPDATED_IV      3
#define  SE_KEYTABLE_DST_WORD_QUAD(x) ((x) << 0)
#define  SE_KEYTABLE_DST_KEY_INDEX(x) ((x) << 8)

#define SE_RNG_CONFIG_REG 0x340
#define  MODE_NORMAL 0
#define  MODE_FORCE_INSTANTION 1
#define  MODE_FORCE_RESEED     2
#define  SE_RNG_CONFIG_MODE(x)        ((x) << 0)
#define  SRC_NONE        0
#define  SRC_ENTROPY     1
#define  SRC_LFSR        2
#define  SE_RNG_CONFIG_SRC(x)         ((x) << 2)

#define SE_RNG_SRC_CONFIG_REG	0x344
#define  RO_ENTR_LOCK_DISABLE  0
#define  RO_ENTR_LOCK_ENABLE   1
#define  SE_RNG_SRC_CONFIG_ENTR_SRC_LOCK(x) ((x) << 0)
#define  RO_ENTR_DISABLE       0
#define  RO_ENTR_ENABLE        1
#define  SE_RNG_SRC_CONFIG_ENTR_SRC(x)      ((x) << 1)
#define  RO_HW_DIS_CYA_DISABLE 0
#define  RO_HW_DIS_CYA_ENABLE  1
#define  SE_RNG_SRC_CONFIG_HW_DIS_CYA(x)    ((x) << 2)
#define  SE_RNG_SRC_CONFIG_ENTR_SUBSMPL(x)  ((x) << 4)
#define  SE_RNG_SRC_CONFIG_ENTR_DATA_FLUSH  BIT(8)

#define SE_RNG_RESEED_INTERVAL_REG 0x348

#define SE_RSA_CONFIG 0x400
#define  RSA_KEY_SLOT_ONE 0
#define  RSA_KEY_SLOT_TW0 1
#define  RSA_KEY_SLOT(x)            ((x) << 24)

#define SE_RSA_KEY_SIZE_REG 0x404
#define  RSA_KEY_WIDTH_512  0
#define  RSA_KEY_WIDTH_1024 1
#define  RSA_KEY_WIDTH_1536 2
#define  RSA_KEY_WIDTH_2048 3

#define SE_RSA_EXP_SIZE_REG 0x408

#define SE_RSA_SECURITY_PERKEY_REG 0x40C
#define  SE_RSA_KEY_LOCK_FLAG               0x80
#define SE_RSA_KEYTABLE_ACCESS_REG 0x410
#define  SE_RSA_KEY_TBL_DIS_KEYREAD_FLAG    BIT(0)
#define  SE_RSA_KEY_TBL_DIS_KEYUPDATE_FLAG  BIT(1)
#define  SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG     BIT(2)
#define  SE_RSA_KEY_TBL_DIS_KEY_ACCESS_FLAG 0x7F
#define  SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_FLAG      (SE_RSA_KEY_TBL_DIS_KEYREAD_FLAG | SE_RSA_KEY_TBL_DIS_KEYUPDATE_FLAG)
#define  SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_USE_FLAG  (SE_RSA_KEY_TBL_DIS_KEYREAD_FLAG | SE_RSA_KEY_TBL_DIS_KEYUPDATE_FLAG | SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG)

#define SE_RSA_KEYTABLE_ADDR_REG 0x420
#define  SE_RSA_KEYTABLE_PKT(x)        ((x) << 0)
#define  RSA_KEY_TYPE_EXP       0
#define  RSA_KEY_TYPE_MOD       1
#define  SE_RSA_KEYTABLE_TYPE(x)       ((x) << 6)
#define  RSA_KEY_NUM(x)                ((x) << 7)
#define  RSA_KEY_INPUT_MODE_REG 0
#define  RSA_KEY_INPUT_MODE_DMA 1
#define  SE_RSA_KEYTABLE_INPUT_MODE(x) ((x) << 8)
#define  RSA_KEY_READ           0
#define  RSA_KEY_WRITE          1
#define  SE_RSA_KEY_OP(x)              ((x) << 10)

#define SE_RSA_KEYTABLE_DATA_REG 0x424

#define SE_RSA_OUTPUT_REG 0x428
#define  SE_RSA_OUTPUT_REG_COUNT 64

#define SE_STATUS_REG 0x800
#define  SE_STATUS_STATE_IDLE     0
#define  SE_STATUS_STATE_BUSY     1
#define  SE_STATUS_STATE_WAIT_OUT 2
#define  SE_STATUS_STATE_WAIT_IN  3
#define  SE_STATUS_STATE_MASK     3

#define SE_ERR_STATUS_REG 0x804
#define  SE_ERR_STATUS_SE_NS_ACCESS    BIT(0)
#define  SE_ERR_STATUS_BUSY_REG_WR     BIT(1)
#define  SE_ERR_STATUS_DST             BIT(2)
#define  SE_ERR_STATUS_SRK_USAGE_LIMIT BIT(3)
#define  SE_ERR_STATUS_TZRAM_NS_ACCESS BIT(24)
#define  SE_ERR_STATUS_TZRAM_ADDRESS   BIT(25)

#define SE_MISC_REG 0x808
#define  SE_ENTROPY_NEXT_192BIT BIT(0)
#define  SE_ENTROPY_VN_BYPASS   BIT(1)
#define  SE_CLK_OVR_ON          BIT(2)

#define SE_SPARE_REG 0x80C
#define  SE_ERRATA_FIX_DISABLE 0
#define  SE_ERRATA_FIX_ENABLE  1
#define  SE_ECO(x) ((x) << 0)

#endif
