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

#include "os.h"
#include "cx.h"
#include "oxen_types.h"
#include "oxen_api.h"
#include "oxen_vars.h"

void update_protocol(void) {
    G_oxen_state.tx_state_ins = G_oxen_state.io_ins;
    G_oxen_state.tx_state_p1 = G_oxen_state.io_p1;
    G_oxen_state.tx_state_p2 = G_oxen_state.io_p2;
}

void clear_protocol(void) {
    G_oxen_state.tx_state_ins = 0;
    G_oxen_state.tx_state_p1 = 0;
    G_oxen_state.tx_state_p2 = 0;
}

int check_protocol(void) {
    /* if locked and pin is verified, unlock */
    if ((G_oxen_state.protocol_barrier == PROTOCOL_LOCKED_UNLOCKABLE) &&
        (os_global_pin_is_validated() == PIN_VERIFIED)) {
        G_oxen_state.protocol_barrier = PROTOCOL_UNLOCKED;
    }

    /* if we are locked, deny all command! */
    if (G_oxen_state.protocol_barrier != PROTOCOL_UNLOCKED) {
        return SW_SECURITY_LOCKED;
    }

    /* the first command enforce the protocol version until application quits */
    switch (G_oxen_state.io_protocol_version) {
        case 0x01: /* protocol V1 */
            if (G_oxen_state.protocol == 0xff) {
                G_oxen_state.protocol = G_oxen_state.io_protocol_version;
            }
            if (G_oxen_state.protocol == G_oxen_state.io_protocol_version) {
                break;
            }
            // unknown protocol or hot protocol switch is not allowed
            __attribute__((fallthrough));
        default:
            return SW_PROTOCOL_NOT_SUPPORTED;
    }
    return SW_OK;
}

int check_ins_access(void) {
    if (G_oxen_state.key_set != 1) {
        return SW_KEY_NOT_SET;
    }

    switch (G_oxen_state.io_ins) {
        case INS_LOCK_DISPLAY:
        case INS_RESET:
        case INS_GET_NETWORK:
        case INS_PUT_KEY:
        case INS_GET_KEY:
        case INS_DISPLAY_ADDRESS:
        case INS_VERIFY_KEY:
        case INS_GET_CHACHA8_PREKEY:
        case INS_GEN_KEY_DERIVATION:
        case INS_DERIVATION_TO_SCALAR:
        case INS_DERIVE_PUBLIC_KEY:
        case INS_DERIVE_SECRET_KEY:
        case INS_GEN_KEY_IMAGE:
        case INS_GEN_KEY_IMAGE_SIGNATURE:
        case INS_GEN_UNLOCK_SIGNATURE:
        case INS_GEN_LNS_SIGNATURE:
        case INS_SECRET_KEY_TO_PUBLIC_KEY:
        case INS_SECRET_KEY_ADD:
        case INS_GENERATE_KEYPAIR:
        case INS_SECRET_SCAL_MUL_KEY:
        case INS_SECRET_SCAL_MUL_BASE:
        case INS_DERIVE_SUBADDRESS_PUBLIC_KEY:
        case INS_GET_SUBADDRESS:
        case INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY:
        case INS_GET_SUBADDRESS_SECRET_KEY:
        case INS_UNBLIND:
        case INS_ENCRYPT_PAYMENT_ID:
        case INS_GET_TX_PROOF:
        case INS_GET_TX_SECRET_KEY:
        case INS_CLOSE_TX:
            return SW_OK;

        case INS_OPEN_TX:
        case INS_SET_SIGNATURE_MODE:
            if (os_global_pin_is_validated() != PIN_VERIFIED) {
                return SW_SECURITY_PIN_LOCKED;
            }
            return SW_OK;

        case INS_GEN_TXOUT_KEYS:
        case INS_PREFIX_HASH:
        case INS_BLIND:
        case INS_VALIDATE:
        case INS_CLSAG:
        case INS_GEN_COMMITMENT_MASK:
            if (os_global_pin_is_validated() != PIN_VERIFIED) {
                return SW_SECURITY_PIN_LOCKED;
            }
            if (G_oxen_state.tx_in_progress != 1) {
                return SW_COMMAND_NOT_ALLOWED;
            }
            return SW_OK;
    }

    return SW_INS_NOT_SUPPORTED;
}

int monero_dispatch(void) {
    int sw;

    if (((sw = check_protocol()) != SW_OK) || ((sw = check_ins_access() != SW_OK))) {
        monero_io_discard(0);
        return sw;
    }

    G_oxen_state.options = monero_io_fetch_u8();

    sw = 0x6F01;

    switch (G_oxen_state.io_ins) {
        case INS_RESET:
            sw = monero_apdu_reset();
            break;
        case INS_LOCK_DISPLAY:
            sw = monero_apdu_lock();
            break;
        case INS_GET_NETWORK:
            sw = monero_apdu_get_network();
            break;
        /* --- KEYS --- */
        case INS_PUT_KEY:
            sw = monero_apdu_put_key();
            break;
        case INS_GET_KEY:
            sw = monero_apdu_get_key();
            break;
        case INS_DISPLAY_ADDRESS:
            sw = monero_apdu_display_address();
            break;

            /* --- PROVISIONING--- */
        case INS_VERIFY_KEY:
            sw = monero_apdu_verify_key();
            break;
        case INS_GET_CHACHA8_PREKEY:
            sw = monero_apdu_get_chacha8_prekey();
            break;
        case INS_SECRET_KEY_TO_PUBLIC_KEY:
            sw = monero_apdu_secret_key_to_public_key();
            break;
        case INS_GEN_KEY_DERIVATION:
            sw = monero_apdu_generate_key_derivation();
            break;
        case INS_DERIVATION_TO_SCALAR:
            sw = monero_apdu_derivation_to_scalar();
            break;
        case INS_DERIVE_PUBLIC_KEY:
            sw = monero_apdu_derive_public_key();
            break;
        case INS_DERIVE_SECRET_KEY:
            sw = monero_apdu_derive_secret_key();
            break;
        case INS_GEN_KEY_IMAGE:
            sw = monero_apdu_generate_key_image();
            break;
        case INS_GEN_KEY_IMAGE_SIGNATURE:
            sw = oxen_apdu_generate_key_image_signature();
            break;
        case INS_SECRET_KEY_ADD:
            sw = monero_apdu_sc_add();
            break;
        case INS_GENERATE_KEYPAIR:
            sw = monero_apdu_generate_keypair();
            break;
        case INS_SECRET_SCAL_MUL_KEY:
            sw = monero_apdu_scal_mul_key();
            break;
        case INS_SECRET_SCAL_MUL_BASE:
            sw = monero_apdu_scal_mul_base();
            break;

        /* --- ADRESSES --- */
        case INS_DERIVE_SUBADDRESS_PUBLIC_KEY:
            sw = monero_apdu_derive_subaddress_public_key();
            break;
        case INS_GET_SUBADDRESS:
            sw = monero_apdu_get_subaddress();
            break;
        case INS_GET_SUBADDRESS_SPEND_PUBLIC_KEY:
            sw = monero_apdu_get_subaddress_spend_public_key();
            break;
        case INS_GET_SUBADDRESS_SECRET_KEY:
            sw = monero_apdu_get_subaddress_secret_key();
            break;

        /* --- PARSE --- */
        case INS_UNBLIND:
            sw = monero_apdu_unblind();
            break;

        /* --- PROOF --- */
        case INS_GET_TX_PROOF:
            sw = monero_apdu_get_tx_proof();
            break;

        /// This call will only work when we have an open transaction *and* it is recognized as a /
        /// stake, but we don't explicitly enforce it to be called at any particular time.
        case INS_GET_TX_SECRET_KEY:
            sw = oxen_apdu_get_tx_secret_key();
            break;

            /* =======================================================================
             *  Following command are only allowed during transaction and their
             *  sequence shall be enforced
             */

            /* --- START TX --- */
        case INS_OPEN_TX:
            // state machine check
            if (!(G_oxen_state.tx_state_ins == 0 ||
                  G_oxen_state.tx_state_ins == INS_GEN_LNS_SIGNATURE)) {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            // 2. command process
            sw = monero_apdu_open_tx();
            update_protocol();
            break;

        case INS_CLOSE_TX:
            sw = monero_apdu_close_tx();
            clear_protocol();
            break;

            /* --- SIG MODE --- */
        case INS_SET_SIGNATURE_MODE:
            // 1. state machine check
            if (G_oxen_state.tx_in_progress != 0) {
                // Change sig mode during transacation is not allowed
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            // 2. command process
            sw = monero_apdu_set_signature_mode();
            break;

            /* --- PAYMENT ID encryption --- */
        case INS_ENCRYPT_PAYMENT_ID:
            // 1. state machine check
            if (G_oxen_state.tx_in_progress == 1) {
                if ((G_oxen_state.tx_state_ins != INS_OPEN_TX) &&
                    (G_oxen_state.tx_state_ins != INS_ENCRYPT_PAYMENT_ID)) {
                    THROW(SW_COMMAND_NOT_ALLOWED);
                }
                if (!OXEN_IO_P_EQUALS(0, 0)) THROW(SW_WRONG_P1P2);
            }
            // 2. command process
            sw = monero_apdu_encrypt_payment_id();
            if (G_oxen_state.tx_in_progress == 1) {
                update_protocol();
            }
            break;

            /* --- TX OUT KEYS --- */
        case INS_GEN_TXOUT_KEYS:
            // 1. state machine check
            if ((G_oxen_state.tx_state_ins != INS_OPEN_TX) &&
                (G_oxen_state.tx_state_ins != INS_GEN_TXOUT_KEYS) &&
                (G_oxen_state.tx_state_ins != INS_ENCRYPT_PAYMENT_ID)) {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            if (!OXEN_IO_P_EQUALS(0, 0)) THROW(SW_WRONG_P1P2);

            // 2. command process
            sw = monero_apu_generate_txout_keys();
            update_protocol();
            break;

        /* --- PREFIX HASH  --- */
        case INS_PREFIX_HASH:
            // init prefixhash state machine if this is the first step:
            if (G_oxen_state.tx_state_ins == INS_GEN_TXOUT_KEYS) {
                G_oxen_state.tx_state_ins = INS_PREFIX_HASH;
                G_oxen_state.tx_state_p1 = 0;
                G_oxen_state.tx_state_p2 = 0;
            } else if (G_oxen_state.tx_state_ins != INS_PREFIX_HASH) {
                // Otherwise we must be coming from a previous INS_PREFIX_HASH step
                THROW(SW_COMMAND_NOT_ALLOWED);
            }

            if (G_oxen_state.tx_state_p1 == 0 && G_oxen_state.io_p1 == 1) {
                // We're going from initialization to our first subcommand, where we get basic tx
                // parameters (version, type, locktime).
                sw = monero_apdu_prefix_hash_init();
            } else if (G_oxen_state.io_p1 == 2 &&
                       (
                           // We've moving from first subcommand to phase 2 where we start receiving
                           // the full prefix; either [2,1] for the first piece of a multi-piece
                           // prefix, or [2,0] for a single-piece prefix:
                           (G_oxen_state.tx_state_p1 == 1 && G_oxen_state.io_p2 <= 1) ||
                           // We're already in phase 2, and moving on to the next piece.  We require
                           // that the next piece index (p2) equals 0 (meaning this is the last
                           // piece), or the old one plus 1.  If we hit 255 then we wrap around to
                           // 1, not 0 (unless the next one just happens to be the last piece).
                           (G_oxen_state.tx_state_p1 == 2 && G_oxen_state.io_p1 == 2 &&
                            (G_oxen_state.io_p2 == 0 ||
                             G_oxen_state.io_p2 == (G_oxen_state.tx_state_p2 < 255
                                                        ? G_oxen_state.tx_state_p2 + 1
                                                        : 1))))) {
                sw = monero_apdu_prefix_hash_update();
            } else {
                // Some invalid subcommand or state transition
                THROW(SW_SUBCOMMAND_NOT_ALLOWED);
            }
            update_protocol();
            break;

            /*--- COMMITMENT MASK --- */
        case INS_GEN_COMMITMENT_MASK:
            if (!OXEN_IO_P_EQUALS(0, 0)) THROW(SW_WRONG_P1P2);

            // 2. command process
            sw = monero_apdu_gen_commitment_mask();
            update_protocol();
            break;

            /* --- BLIND --- */
        case INS_BLIND:
            // 1. state machine check
            if (G_oxen_state.tx_sig_mode == TRANSACTION_CREATE_FAKE) {
            } else if (G_oxen_state.tx_sig_mode == TRANSACTION_CREATE_REAL) {
                if ((G_oxen_state.tx_state_ins != INS_GEN_COMMITMENT_MASK) &&
                    (G_oxen_state.tx_state_ins != INS_BLIND)) {
                    THROW(SW_COMMAND_NOT_ALLOWED);
                }
            } else {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            // 2. command process
            if ((G_oxen_state.io_p1 != 0) || (G_oxen_state.io_p2 != 0)) {
                THROW(SW_WRONG_P1P2);
            }
            sw = monero_apdu_blind();
            update_protocol();
            break;

            /* --- VALIDATE/PREHASH --- */
        case INS_VALIDATE:
            // 1. state machine check
            if ((G_oxen_state.tx_state_ins != INS_BLIND) &&
                (G_oxen_state.tx_state_ins != INS_VALIDATE)) {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            // init PREHASH state machine
            if (G_oxen_state.tx_state_ins == INS_BLIND) {
                G_oxen_state.tx_state_ins = INS_VALIDATE;
                G_oxen_state.tx_state_p1 = 1;
                G_oxen_state.tx_state_p2 = 0;
                if ((G_oxen_state.io_p1 != 1) || (G_oxen_state.io_p2 != 1)) {
                    THROW(SW_SUBCOMMAND_NOT_ALLOWED);
                }
            }
            // check new state is allowed
            if (G_oxen_state.tx_state_p1 == G_oxen_state.io_p1) {
                if (G_oxen_state.tx_state_p2 != G_oxen_state.io_p2 - 1) {
                    THROW(SW_SUBCOMMAND_NOT_ALLOWED);
                }
            } else if (G_oxen_state.tx_state_p1 == G_oxen_state.io_p1 - 1) {
                if (1 != G_oxen_state.io_p2) {
                    THROW(SW_SUBCOMMAND_NOT_ALLOWED);
                }
            } else {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }
            // 2. command process
            if (G_oxen_state.io_p1 == 1) {
                sw = monero_apdu_clsag_prehash_init();
            } else if (G_oxen_state.io_p1 == 2) {
                sw = monero_apdu_clsag_prehash_update();
            } else if (G_oxen_state.io_p1 == 3) {
                sw = monero_apdu_clsag_prehash_finalize();
            } else {
                THROW(SW_WRONG_P1P2);
            }
            update_protocol();
            break;

        /* --- CLSAG --- */
        case INS_CLSAG:
            // If we are going to [CLSAG, 1, 0] then we must be coming from either [VALIDATE, 3] or
            // [CLSAG, 3, 0]
            if (OXEN_IO_P_EQUALS(1, 0)) {
                if ((G_oxen_state.tx_state_ins == INS_VALIDATE && G_oxen_state.tx_state_p1 == 3) ||
                    OXEN_TX_STATE_INS_P_EQUALS(INS_CLSAG, 3, 0))
                    sw = monero_apdu_clsag_prepare();
                else
                    THROW(SW_SUBCOMMAND_NOT_ALLOWED);
            } else if (G_oxen_state.tx_state_ins == INS_CLSAG) {
                // Transitioning between CLSAG states

                // [1,0]->[2,x] or [2,x]->[2,y] (oxen_clsag.c does x/y validation)
                if ((OXEN_TX_STATE_P_EQUALS(1, 0) || G_oxen_state.tx_state_p1 == 2) &&
                    G_oxen_state.io_p1 == 2)
                    sw = monero_apdu_clsag_hash();

                // [2,0]->[3,0] - sign
                else if (OXEN_TX_STATE_P_EQUALS(2, 0) && OXEN_IO_P_EQUALS(3, 0))
                    sw = monero_apdu_clsag_sign();

                else
                    THROW(SW_SUBCOMMAND_NOT_ALLOWED);
            } else {
                THROW(SW_COMMAND_NOT_ALLOWED);
            }

            update_protocol();
            break;

        /* --- Unlock --- */
        case INS_GEN_UNLOCK_SIGNATURE:
            if (G_oxen_state.tx_in_progress) THROW(SW_COMMAND_NOT_ALLOWED);
            // Initialization: must not be in the middle of something else
            if (OXEN_IO_P_EQUALS(0, 0) && G_oxen_state.tx_state_ins == 0)
                sw = oxen_apdu_generate_unlock_signature();
            else if (OXEN_IO_P_EQUALS(1, 0) &&
                     OXEN_TX_STATE_INS_P_EQUALS(INS_GEN_UNLOCK_SIGNATURE, 0, 0))
                // [UNLOCK,1,0] is the post-confirmation step and must follow immediately the
                // [UNLOCK,0,0] (which is where we ask for confirmation).
                sw = oxen_apdu_generate_unlock_signature();
            else
                THROW(SW_COMMAND_NOT_ALLOWED);

            update_protocol();
            break;

        /* --- LNS --- */
        case INS_GEN_LNS_SIGNATURE:
            if (G_oxen_state.tx_in_progress) THROW(SW_COMMAND_NOT_ALLOWED);
            // Initialization: must not be in the middle of something else
            if (OXEN_IO_P_EQUALS(0, 0) && G_oxen_state.tx_state_ins == 0)
                sw = oxen_apdu_generate_lns_hash();
            // [0,0]->[1,x] or [1,x]->[1,y] -- we receive data to hash in multiple parts
            else if (G_oxen_state.tx_state_ins == INS_GEN_LNS_SIGNATURE &&
                     (OXEN_TX_STATE_P_EQUALS(0, 0) || G_oxen_state.tx_state_p1 == 1) &&
                     G_oxen_state.io_p1 == 1)
                sw = oxen_apdu_generate_lns_hash();
            // [1,0] -> [2,0] gives the account indices and uses the hash built in the [1,x] steps
            // to make the signature
            else if (OXEN_TX_STATE_INS_P_EQUALS(INS_GEN_LNS_SIGNATURE, 1, 0) &&
                     OXEN_IO_P_EQUALS(2, 0))
                sw = oxen_apdu_generate_lns_signature();
            else
                THROW(SW_COMMAND_NOT_ALLOWED);

            update_protocol();
            break;

        default:
            THROW(SW_INS_NOT_SUPPORTED);
            break;
    }
    return sw;
}
