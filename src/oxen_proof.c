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

/* ----------------------------------------------------------------------- */
/* ---                                                                 --- */
/* ----------------------------------------------------------------------- */
int monero_apdu_get_tx_proof(void) {
    unsigned char *msg;
    unsigned char *R;
    unsigned char *A;
    unsigned char *B;
    unsigned char *D;
    unsigned char r[32];
    unsigned char XY[32];
    unsigned char sig_c[32];
    unsigned char sig_r[32];
#define k (G_oxen_state.tmp + 128)  // We go 32 bytes into this

    msg = G_oxen_state.io_buffer + G_oxen_state.io_offset;
    monero_io_fetch(NULL, 32);
    R = G_oxen_state.io_buffer + G_oxen_state.io_offset;
    monero_io_fetch(NULL, 32);
    A = G_oxen_state.io_buffer + G_oxen_state.io_offset;
    monero_io_fetch(NULL, 32);
    B = G_oxen_state.io_buffer + G_oxen_state.io_offset;
    monero_io_fetch(NULL, 32);
    D = G_oxen_state.io_buffer + G_oxen_state.io_offset;
    monero_io_fetch(NULL, 32);
    monero_io_fetch_decrypt_key(r);

    monero_io_discard(0);

    // Generate random k
    monero_rng_mod_order(k);
    // tmp = msg
    memmove(G_oxen_state.tmp + 32 * 0, msg, 32);
    // tmp = msg || D
    memmove(G_oxen_state.tmp + 32 * 1, D, 32);

    if (G_oxen_state.options & 1) {
        // X = kB
        monero_ecmul_k(XY, B, k);
    } else {
        // X = kG
        monero_ecmul_G(XY, k);
    }
    // tmp = msg || D || X
    memmove(G_oxen_state.tmp + 32 * 2, XY, 32);

    // Y = kA
    monero_ecmul_k(XY, A, k);
    // tmp = msg || D || X || Y
    memmove(G_oxen_state.tmp + 32 * 3, XY, 32);

    /* Monero V2 proofs (not currently present in Oxen).
     *
     * Note that changing this will require enlarging tmp!

        oxen_keccak_256(&G_oxen_state.keccak_alt, (unsigned char *)"TXPROOF_V2", 10, sep);
        // tmp = msg || D || X || Y || sep
        memmove(G_oxen_state.tmp + 32 * 4, sep, 32);
        // tmp = msg || D || X || Y || sep || R
        memmove(G_oxen_state.tmp + 32 * 5, R, 32);
        // tmp = msg || D || X || Y || sep || R || A
        memmove(G_oxen_state.tmp + 32 * 6, A, 32);
        // tmp = msg || D || X || Y || sep || R || B or [0]
        memmove(G_oxen_state.tmp + 32 * 7, B, 32);

        monero_hash_to_scalar(sig_c, &G_oxen_state.tmp[0], 32 * 8);
    */
    (void) R;

    // sig_c = H_n(tmp)
    monero_hash_to_scalar(sig_c, &G_oxen_state.tmp[0], 32 * 4);
    // Monero V2 proof version:
    // monero_hash_to_scalar(sig_c, &G_oxen_state.tmp[0], 32 * 8);

    // sig_c*r
    monero_multm(XY, sig_c, r);
    // sig_r = k - sig_c*r
    monero_subm(sig_r, k, XY);

    monero_io_insert(sig_c, 32);
    monero_io_insert(sig_r, 32);

    return SW_OK;
}
