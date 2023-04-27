/*****************************************************************************
 *   Ledger Oxen App.
 *   (c) 2017-2020 Cedric Mesnil <cslashm@gmail.com>, Ledger SAS.
 *   (c) 2020 Ledger SAS.
 *   (c) 2020 Oxen Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#ifndef OXEN_TYPES_H
#define OXEN_TYPES_H

#include "os_io_seproxyhal.h"
#ifdef TARGET_NANOS
#include "lcx_blake2.h"
#include "lcx_sha3.h"
#include "lcx_sha256.h"
#include "lcx_aes.h"
#elif defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#include "cx.h"
#else
#error "Error: neither TARGET_NANOS neither TARGET_NANOX is defined, don't know what to include!"
#endif

#if CX_APILEVEL == 8
#define PIN_VERIFIED (!0)
#elif CX_APILEVEL >= 9

#define PIN_VERIFIED BOLOS_UX_OK
#else
#error CX_APILEVEL not  supported
#endif

/* cannot send more that F0 bytes in CCID, why? do not know for now
 *  So set up length to F0 minus 2 bytes for SW
 */
#define MONERO_APDU_LENGTH 0xFE

/* --- ... --- */
#define MAINNET_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX            114
#define MAINNET_CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX 115
#define MAINNET_CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX         116

#define DEVNET_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX            3930
#define DEVNET_CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX 4442
#define DEVNET_CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX         5850

#define TESTNET_CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX            156
#define TESTNET_CRYPTONOTE_PUBLIC_INTEGRATED_ADDRESS_BASE58_PREFIX 157
#define TESTNET_CRYPTONOTE_PUBLIC_SUBADDRESS_BASE58_PREFIX         158

#define COIN_DECIMAL 9  // 1 OXEN = 1'000'000'000 atomic OXEN

enum network_type {
#ifndef OXEN_ALPHA
    MAINNET = 0,
#endif
    TESTNET = 1,
    DEVNET = 2,
    FAKECHAIN = 3
};

typedef struct oxen_nv_state_t {
    /* magic */
    unsigned char magic[8];

    /* network */
    unsigned char network_id;

/* key mode */
#define KEY_MODE_EXTERNAL 0x21
#define KEY_MODE_SEED     0x42
    unsigned char key_mode;

#define VIEWKEY_EXPORT_ALWAYS_PROMPT 0
#define VIEWKEY_EXPORT_ALWAYS_ALLOW  1
#define VIEWKEY_EXPORT_ALWAYS_DENY   2
    /* view key export mode */
    unsigned char viewkey_export_mode;

#define CONFIRM_ADDRESS_FULL    0  // Full addr (takes 6-7 pages)
#define CONFIRM_ADDRESS_SHORT   1  // [first 23..last 23] (so fits on 3 pages)
#define CONFIRM_ADDRESS_SHORTER 2  // [first 16..last 14] (so fits on 2 pages)
    /* address confirm truncatation mode */
    unsigned char truncate_addrs_mode;

#define CONFIRM_FEE_ALWAYS     0
#define CONFIRM_FEE_ABOVE_0_05 1
#define CONFIRM_FEE_ABOVE_0_2  2
#define CONFIRM_FEE_ABOVE_1    3
#define CONFIRM_FEE_NEVER      4
    /* fee confirmation mode */
    unsigned char confirm_fee_mode;

#define CONFIRM_CHANGE_DISABLED 0
#define CONFIRM_CHANGE_ENABLED  1
    /* confirm confirmation mode */
    unsigned char confirm_change_mode;

    /* spend key */
    unsigned char spend_priv[32];
    /* view key */
    unsigned char view_priv[32];

/*words*/
#define WORDS_MAX_LENGTH 20
    union {
        char words[26][WORDS_MAX_LENGTH];
        char words_list[25 * WORDS_MAX_LENGTH + 25];
    };
} oxen_nv_state_t;

enum device_mode { NONE, TRANSACTION_CREATE_REAL, TRANSACTION_CREATE_FAKE, TRANSACTION_PARSE };

enum txtype { TXTYPE_STANDARD, TXTYPE_STATE_CHANGE, TXTYPE_UNLOCK, TXTYPE_STAKE, TXTYPE_LNS };

#define DISP_MAIN       0x51
#define DISP_SUB        0x52
#define DISP_INTEGRATED 0x53

#define MONERO_IO_BUFFER_LENGTH 288

typedef struct oxen_v_state_t {
    unsigned char state;
    unsigned char protocol;

    /* ------------------------------------------ */
    /* ---                  IO                --- */
    /* ------------------------------------------ */

    /* io state*/
    unsigned char io_protocol_version;
    unsigned char io_ins;
    unsigned char io_p1;
    unsigned char io_p2;
    unsigned char io_lc;
    unsigned char io_le;
    unsigned short io_length;
    unsigned short io_offset;
    unsigned char io_buffer[MONERO_IO_BUFFER_LENGTH];

    unsigned char options;

    /* ------------------------------------------ */
    /* ---            State Machine           --- */
    /* ------------------------------------------ */

/* protocol guard */
#define PROTOCOL_LOCKED            0x42
#define PROTOCOL_LOCKED_UNLOCKABLE 0x84
#define PROTOCOL_UNLOCKED          0x24
    unsigned char protocol_barrier;

    unsigned char export_view_key : 1;
    unsigned char key_set : 1;

    /* Tx state machine */
    unsigned char tx_in_progress : 1;
    unsigned char tx_special_confirmed : 1;
    unsigned char tx_type;
    unsigned char tx_cnt;
    unsigned char tx_sig_mode;
    unsigned char tx_state_ins;
    unsigned char tx_state_p1;
    unsigned char tx_state_p2;
    unsigned char tx_output_cnt;
    unsigned int tx_sign_cnt;

    /* sc_add control */
    unsigned char last_derive_secret_key[32];
    unsigned char last_get_subaddress_secret_key[32];

    /* ------------------------------------------ */
    /* ---               Crypto               --- */
    /* ------------------------------------------ */
    union {
        struct {
            unsigned char view_priv[32];
            unsigned char view_pub[32];
            unsigned char spend_priv[32];
            unsigned char spend_pub[32];
        };
        unsigned char keys[128];
    };

    /* SPK */
    cx_aes_key_t spk;
    unsigned char hmac_key[32];

    /* Tx key */
    unsigned char R[32];
    unsigned char r[32];

    /* hashing */
    union {
        cx_sha3_t keccak;
        cx_blake2b_t blake2b;
    };
    cx_sha3_t keccak_alt;
    cx_sha256_t sha256;
    cx_sha256_t sha256_alt;
    unsigned char prefixH[32];
    union {
        unsigned char clsag_c[32];
        unsigned char commitment_hash[32];
        unsigned char addr_checksum_hash[32];
        unsigned char lns_hash[32];
    };

    /* -- track tx-in/out -- */
    unsigned char OUTK[32];

    /* ------------------------------------------ */
    /* ---               UI/UX                --- */
    /* ------------------------------------------ */
    char ux_wallet_public_short_address[7 + 2 + 3 + 1];  // first 7, two dots, last 3, null

    union {
        struct {
            char ux_info1[17];
            char ux_info2[17];
        };
        struct {
            // address to display: 95/106-chars for mainnet, 97/108 for testnet + null
            char ux_address[109];
            // Address type string; max 15 chars + null
            char ux_addr_type[16];
            // addr mode
            unsigned char disp_addr_mode;
            union {
                // Address extra info (e.g. maj/min; payment id); max 31 chars + null
                char ux_addr_info[32];
                // Payment ID, which gets copied into the above for display
                char payment_id[16];
                struct {
                    // Major/minor addr indicies for subaddresses (copied into ux_addr_info for
                    // display)
                    unsigned int disp_addr_M;
                    unsigned int disp_addr_m;
                };
            };
            // OXEN to display: max pow(2,64) uint, aka 20-chars + dot + null (there can also be a
            // 0, but won't be if the value is greater than 9 digits).
            char ux_amount[22];
        };
        unsigned char tmp[160];  // Used as extra temp storage in oxen_proof, oxen_clsag
    };
} oxen_v_state_t;

#define STATE_IDLE 0xC0

/* --- ... --- */
#define TYPE_SCALAR     1
#define TYPE_DERIVATION 2
#define TYPE_AMOUNT_KEY 3
#define TYPE_ALPHA      4

/* ---  ...  --- */
#define IO_OFFSET_END (unsigned int) -1

#define ENCRYPTED_PAYMENT_ID_TAIL 0x8d

/* ---  Errors  --- */
#define ERROR(x) ((x) << 16)

#define ERROR_IO_OFFSET ERROR(1)
#define ERROR_IO_FULL   ERROR(2)

/* ---  INS  --- */

#define INS_RESET        0x02
#define INS_LOCK_DISPLAY 0x04

#define INS_GET_NETWORK   0x10
#define INS_RESET_NETWORK 0x11

#define INS_GET_KEY            0x20
#define INS_DISPLAY_ADDRESS    0x21
#define INS_PUT_KEY            0x22
#define INS_GET_CHACHA8_PREKEY 0x24
#define INS_VERIFY_KEY         0x26

#define INS_SECRET_KEY_TO_PUBLIC_KEY 0x30
#define INS_GEN_KEY_DERIVATION       0x32
#define INS_DERIVATION_TO_SCALAR     0x34
#define INS_DERIVE_PUBLIC_KEY        0x36
#define INS_DERIVE_SECRET_KEY        0x38
#define INS_GEN_KEY_IMAGE            0x3A
#define INS_SECRET_KEY_ADD           0x3C
#define INS_GENERATE_KEYPAIR         0x40
#define INS_SECRET_SCAL_MUL_KEY      0x42
#define INS_SECRET_SCAL_MUL_BASE     0x44

#define INS_DERIVE_SUBADDRESS_PUBLIC_KEY    0x46
#define INS_GET_SUBADDRESS                  0x48
#define INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY 0x4A
#define INS_GET_SUBADDRESS_SECRET_KEY       0x4C

#define INS_OPEN_TX             0x70
#define INS_SET_SIGNATURE_MODE  0x72
#define INS_GET_ADDITIONAL_KEY  0x74
#define INS_GET_TX_SECRET_KEY   0x75
#define INS_ENCRYPT_PAYMENT_ID  0x76
#define INS_GEN_COMMITMENT_MASK 0x77
#define INS_BLIND               0x78
#define INS_UNBLIND             0x7A
#define INS_GEN_TXOUT_KEYS      0x7B
#define INS_PREFIX_HASH         0x7D
#define INS_VALIDATE            0x7C
#define INS_CLSAG               0x7F
#define INS_CLOSE_TX            0x80

#define INS_GET_TX_PROOF            0xA0
#define INS_GEN_UNLOCK_SIGNATURE    0xA2
#define INS_GEN_LNS_SIGNATURE       0xA3
#define INS_GEN_KEY_IMAGE_SIGNATURE 0xA4

#define INS_GET_RESPONSE 0xc0

/* --- OPTIONS --- */
#define IN_OPTION_MASK  0x000000FF
#define OUT_OPTION_MASK 0x0000FF00

#define IN_OPTION_MORE_COMMAND 0x00000080

/* ---  IO constants  --- */
#define OFFSET_CLA       0
#define OFFSET_INS       1
#define OFFSET_P1        2
#define OFFSET_P2        3
#define OFFSET_P3        4
#define OFFSET_CDATA     5
#define OFFSET_EXT_CDATA 7

#define SW_OK 0x9000

#define SW_WRONG_LENGTH 0x6700

#define SW_SECURITY_PIN_LOCKED               0x6910
#define SW_SECURITY_LOAD_KEY                 0x6911
#define SW_SECURITY_COMMITMENT_CONTROL       0x6912
#define SW_SECURITY_AMOUNT_CHAIN_CONTROL     0x6913
#define SW_SECURITY_COMMITMENT_CHAIN_CONTROL 0x6914
#define SW_SECURITY_OUTKEYS_CHAIN_CONTROL    0x6915
#define SW_SECURITY_MAXOUTPUT_REACHED        0x6916
#define SW_SECURITY_HMAC                     0x6917
#define SW_SECURITY_RANGE_VALUE              0x6918
#define SW_SECURITY_INTERNAL                 0x6919
#define SW_SECURITY_MAX_SIGNATURE_REACHED    0x691A
#define SW_SECURITY_PREFIX_HASH              0x691B
#define SW_SECURITY_LOCKED                   0x69EE

#define SW_COMMAND_NOT_ALLOWED    0x6980
#define SW_SUBCOMMAND_NOT_ALLOWED 0x6981
#define SW_DENY                   0x6982
#define SW_KEY_NOT_SET            0x6983
#define SW_WRONG_DATA             0x6984
#define SW_WRONG_DATA_RANGE       0x6985
#define SW_IO_FULL                0x6986

#define SW_CLIENT_NOT_SUPPORTED 0x6A30

#define SW_WRONG_P1P2             0x6b00
#define SW_INS_NOT_SUPPORTED      0x6d00
#define SW_PROTOCOL_NOT_SUPPORTED 0x6e00

#define SW_UNKNOWN 0x6f00

#endif
