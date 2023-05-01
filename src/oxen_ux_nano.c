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

#include "os_io_seproxyhal.h"
#include "glyphs.h"

/* ----------------------------------------------------------------------- */
/* ---                     Nano S and X UI layout                      --- */
/* ----------------------------------------------------------------------- */

#define ACCEPT 0xACCE
#define REJECT ~ACCEPT

/* -------------------------------------- LOCK--------------------------------------- */

void ui_menu_pinlock_display(void) {
    struct {
        bolos_ux_t ux_id;
        // length of parameters in the u union to be copied during the syscall
        unsigned int len;
        union {
            struct {
                unsigned int cancellable;
            } validate_pin;
        } u;

    } ux_params;

    os_global_pin_invalidate();
    G_oxen_state.protocol_barrier = PROTOCOL_LOCKED_UNLOCKABLE;
    ux_params.ux_id = BOLOS_UX_VALIDATE_PIN;
    ux_params.len = sizeof(ux_params.u.validate_pin);
    ux_params.u.validate_pin.cancellable = 0;
    os_ux((bolos_ux_params_t*) &ux_params);
    ui_menu_main_display();
}

// Ordered list of Settings items, so that we can return to the same item on the settings page from
// a settings subpage.
#define UI_SETTINGS_BACK_TO_MAIN    0
#define UI_SETTINGS_VIEW_KEY_EXPORT 1
#define UI_SETTINGS_FEE_CONFIRM     2
#define UI_SETTINGS_ADDRESS_CONFIRM 3
#define UI_SETTINGS_CHANGE_CONFIRM  4
#define UI_SETTINGS_SELECT_NETWORK  5
#define UI_SETTINGS_RESET           6
#define UI_SETTINGS_BACK            7

void ui_menu_settings_display(void);
void ui_menu_settings_display_select(unsigned int idx);

/* -------------------------------- INFO UX --------------------------------- */
unsigned int ui_menu_info_action(void);

UX_STEP_CB(ux_menu_info_show_step,
           nn,
           ui_menu_info_action(),
           {G_oxen_state.ux_info1, G_oxen_state.ux_info2});
UX_FLOW(ux_flow_info, &ux_menu_info_show_step);

UX_STEP_CB(ux_menu_info_icon1_step,
           pb,
           ui_menu_info_action(),
           {&C_icon_tx1, G_oxen_state.ux_info1});
UX_STEP_CB(ux_menu_info_icon2_step,
           pb,
           ui_menu_info_action(),
           {&C_icon_tx2, G_oxen_state.ux_info1});
UX_STEP_CB(ux_menu_info_icon3_step,
           pb,
           ui_menu_info_action(),
           {&C_icon_tx3, G_oxen_state.ux_info1});
UX_STEP_CB(ux_menu_info_icon4_step,
           pb,
           ui_menu_info_action(),
           {&C_icon_tx4, G_oxen_state.ux_info1});
UX_FLOW(ux_flow_info_icon1, &ux_menu_info_icon1_step);
UX_FLOW(ux_flow_info_icon2, &ux_menu_info_icon2_step);
UX_FLOW(ux_flow_info_icon3, &ux_menu_info_icon3_step);
UX_FLOW(ux_flow_info_icon4, &ux_menu_info_icon4_step);

unsigned int ui_menu_info_action(void) {
    if (G_oxen_state.protocol_barrier == PROTOCOL_LOCKED)
        ui_menu_pinlock_display();
    else
        ui_menu_main_display();
    return 0;
}

void ui_menu_info_display2(const char* line1, const char* line2) {
    memmove(G_oxen_state.ux_info1, line1, sizeof(G_oxen_state.ux_info1) - 1);
    memmove(G_oxen_state.ux_info2, line2, sizeof(G_oxen_state.ux_info2) - 1);
    G_oxen_state.ux_info1[sizeof(G_oxen_state.ux_info1) - 1] = 0;
    G_oxen_state.ux_info2[sizeof(G_oxen_state.ux_info2) - 1] = 0;
    ux_flow_init(0, ux_flow_info, NULL);
}

void ui_menu_info_display(void) {
    ux_flow_init(0, ux_flow_info, NULL);
}

/* -------------------------------- OPEN TX UX --------------------------------- */
static const char* processing_tx(void) {
    return G_oxen_state.tx_type == TXTYPE_STAKE    ? "Processing Stake"
           : G_oxen_state.tx_type == TXTYPE_ONS    ? "Processing ONS"
           : G_oxen_state.tx_type == TXTYPE_UNLOCK ? "Processing Unlck"
                                                   : "Processing TX";
}

void ui_menu_opentx_display(unsigned char final_step) {
    memmove(G_oxen_state.ux_info1, processing_tx(), sizeof(G_oxen_state.ux_info1) - 1);
    G_oxen_state.ux_info1[sizeof(G_oxen_state.ux_info1) - 1] = 0;

    if (final_step)
        ux_flow_init(0, ux_flow_info_icon4, NULL);
    else if (G_oxen_state.tx_cnt % 3 == 1)
        ux_flow_init(0, ux_flow_info_icon1, NULL);
    else if (G_oxen_state.tx_cnt % 3 == 2)
        ux_flow_init(0, ux_flow_info_icon2, NULL);
    else
        ux_flow_init(0, ux_flow_info_icon3, NULL);
}

/* ----------------- FEE/CHANGE/TIMELOCK VALIDATION ----------------- */

void ui_menu_amount_validation_action(unsigned int value);

#define OXEN_UX_CONFIRM_AMOUNT_STEP(name, title) \
    UX_STEP_NOCB(name, bn, {title, G_oxen_state.ux_amount})

OXEN_UX_CONFIRM_AMOUNT_STEP(ux_menu_validation_fee_step, "Confirm Fee");
OXEN_UX_CONFIRM_AMOUNT_STEP(ux_menu_validation_lns_fee_step, "Confirm ONS Fee");
OXEN_UX_CONFIRM_AMOUNT_STEP(ux_menu_validation_change_step, "Amount (change)");
OXEN_UX_CONFIRM_AMOUNT_STEP(ux_menu_validation_timelock_step, "Timelock");

#define OXEN_UX_ACCEPT_REJECT(basename, callback)                                              \
    UX_STEP_CB(basename##_accept_step, pb, callback(ACCEPT), {&C_icon_validate_14, "Accept"}); \
    UX_STEP_CB(basename##_reject_step, pb, callback(REJECT), {&C_icon_crossmark, "Reject"})

OXEN_UX_ACCEPT_REJECT(ux_menu_validation_cf, ui_menu_amount_validation_action);

UX_FLOW(ux_flow_fee,
        &ux_menu_validation_fee_step,
        &ux_menu_validation_cf_accept_step,
        &ux_menu_validation_cf_reject_step);

UX_FLOW(ux_flow_lns_fee,
        &ux_menu_validation_lns_fee_step,
        &ux_menu_validation_cf_accept_step,
        &ux_menu_validation_cf_reject_step);

UX_FLOW(ux_flow_change,
        &ux_menu_validation_change_step,
        &ux_menu_validation_cf_accept_step,
        &ux_menu_validation_cf_reject_step);

UX_FLOW(ux_flow_timelock,
        &ux_menu_validation_timelock_step,
        &ux_menu_validation_cf_accept_step,
        &ux_menu_validation_cf_reject_step);

void ui_menu_amount_validation_action(unsigned int value) {
    unsigned short sw;
    if (value == ACCEPT) {
        sw = SW_OK;
    } else {
        monero_abort_tx();
        sw = SW_DENY;
    }
    monero_io_insert_u16(sw);
    monero_io_do(IO_RETURN_AFTER_TX);
    ui_menu_opentx_display(1);
}

void ui_menu_fee_validation_display(void) {
    ux_flow_init(0, ux_flow_fee, NULL);
}
void ui_menu_lns_fee_validation_display(void) {
    ux_flow_init(0, ux_flow_lns_fee, NULL);
}
void ui_menu_change_validation_display(void) {
    ux_flow_init(0, ux_flow_change, NULL);
}
void ui_menu_timelock_validation_display(void) {
    ux_flow_init(0, ux_flow_timelock, NULL);
}

/* ----------------------------- USER DEST/AMOUNT VALIDATION ----------------------------- */
void ui_menu_validation_action(unsigned int value);

UX_STEP_NOCB(ux_menu_validation_amount_step, bn, {"Confirm Amount", G_oxen_state.ux_amount});

UX_STEP_NOCB(ux_menu_validation_recipient_step,
             bnnn_paging,
             {"Recipient", G_oxen_state.ux_address});

OXEN_UX_ACCEPT_REJECT(ux_menu_validation, ui_menu_validation_action);

UX_FLOW(ux_flow_validation,
        &ux_menu_validation_amount_step,
        &ux_menu_validation_recipient_step,
        &ux_menu_validation_accept_step,
        &ux_menu_validation_reject_step,
        FLOW_LOOP);

void ui_menu_validation_display(void) {
    ux_flow_init(0, ux_flow_validation, NULL);
}

void ui_menu_validation_action(unsigned int value) {
    unsigned short sw;
    if (value == ACCEPT) {
        sw = SW_OK;
    } else {
        monero_abort_tx();
        sw = SW_DENY;
    }
    monero_io_insert_u16(sw);
    monero_io_do(IO_RETURN_AFTER_TX);
    ui_menu_opentx_display(1);
}

// Same as above, but for stake txes: we don't need to show the recipient (because we have already
// enforced that this wallet is the recipient).
UX_STEP_NOCB(ux_menu_stake_validation_step, bn, {"Confirm Stake", G_oxen_state.ux_amount});

UX_FLOW(ux_flow_stake_validation,
        &ux_menu_stake_validation_step,
        &ux_menu_validation_accept_step,
        &ux_menu_validation_reject_step);

void ui_menu_stake_validation_display(void) {
    ux_flow_init(0, ux_flow_stake_validation, NULL);
}

/** Common menu items for special transaction (i.e. unlocks and ONS) */
void ui_menu_special_validation_action(unsigned int value) {
    unsigned short sw;
    if (value == ACCEPT) {
        G_oxen_state.tx_special_confirmed = 1;
        sw = SW_OK;
    } else {
        clear_protocol();
        sw = SW_DENY;
    }
    monero_io_insert_u16(sw);
    monero_io_do(IO_RETURN_AFTER_TX);
    ui_menu_main_display();
}
OXEN_UX_ACCEPT_REJECT(ux_menu_special_validation, ui_menu_special_validation_action);

/* Sign unlock output */
UX_STEP_NOCB(ux_menu_unlock_validation_step, nn, {"Confirm Service", "Node Unlock"});
UX_FLOW(ux_flow_unlock_validation,
        &ux_menu_unlock_validation_step,
        &ux_menu_special_validation_accept_step,
        &ux_menu_special_validation_reject_step);
void ui_menu_unlock_validation_display(void) {
    ux_flow_init(0, ux_flow_unlock_validation, NULL);
}

/* ONS */
UX_STEP_NOCB(ux_menu_lns_validation_step, nn, {"Confirm Oxen", "Name Service TX"});
UX_FLOW(ux_flow_lns_validation,
        &ux_menu_lns_validation_step,
        &ux_menu_special_validation_accept_step,
        &ux_menu_special_validation_reject_step);

void ui_menu_lns_validation_display(void) {
    ux_flow_init(0, ux_flow_lns_validation, NULL);
}

/* -------------------------------- EXPORT VIEW KEY UX --------------------------------- */
unsigned int ui_menu_export_viewkey_action(unsigned int value);

UX_STEP_CB(ux_menu_export_viewkey_export_step,
           pb,
           ui_menu_export_viewkey_action(ACCEPT),
           {&C_icon_validate_14, "Export view key?"});

UX_STEP_CB(ux_menu_export_viewkey_export_always_step,
           pb,
           ui_menu_export_viewkey_action(ACCEPT | 0x10000),
           {&C_icon_validate_14, "Always export"});

UX_STEP_CB(ux_menu_export_viewkey_reject_step,
           pb,
           ui_menu_export_viewkey_action(REJECT),
           {&C_icon_crossmark, "Reject"});

UX_STEP_CB(ux_menu_export_viewkey_reject_always_step,
           pb,
           ui_menu_export_viewkey_action(REJECT | 0x10000),
           {&C_icon_crossmark, "Always reject"});

UX_FLOW(ux_flow_export_viewkey,
        &ux_menu_export_viewkey_export_step,
        &ux_menu_export_viewkey_export_always_step,
        &ux_menu_export_viewkey_reject_step,
        &ux_menu_export_viewkey_reject_always_step,
        FLOW_LOOP);

void ui_export_viewkey_display(void) {
    switch (N_oxen_state->viewkey_export_mode) {
        case VIEWKEY_EXPORT_ALWAYS_ALLOW:
            ui_menu_export_viewkey_action(ACCEPT);
            break;
        case VIEWKEY_EXPORT_ALWAYS_DENY:
            ui_menu_export_viewkey_action(REJECT);
            break;
        default:
            ux_flow_init(0, ux_flow_export_viewkey, NULL);
            break;
    }
}

unsigned int ui_menu_export_viewkey_action(unsigned int value) {
    unsigned int sw;
    unsigned char x[32];

    monero_io_discard(0);
    memset(x, 0, 32);
    sw = SW_OK;
    if (value & 0x10000) {  // remember
        value &= ~0x10000;
        unsigned char val =
            value == ACCEPT ? VIEWKEY_EXPORT_ALWAYS_ALLOW : VIEWKEY_EXPORT_ALWAYS_DENY;
        nvm_write((void*) &N_oxen_state->viewkey_export_mode, &val, sizeof(unsigned char));
    }

    if (value == ACCEPT) {
        monero_io_insert(G_oxen_state.view_priv, 32);
        G_oxen_state.export_view_key = 1;
    } else {
        monero_io_insert(x, 32);
        G_oxen_state.export_view_key = 0;
    }
    monero_io_insert_u16(sw);
    monero_io_do(IO_RETURN_AFTER_TX);
    ui_menu_main_display();
    return 0;
}

// NB: indices need to match up with the VIEWKEY_EXPORT_... constants:
const char* const viewkey_export_submenu_values[] = {"Always prompt",
                                                     "Always allow",
                                                     "Always deny"};
const char* const viewkey_export_submenu_values_selected[] = {"Always prompt *",
                                                              "Always allow *",
                                                              "Always deny *"};

const char* viewkey_export_submenu_getter(unsigned int idx) {
    if (idx >= ARRAYLEN(viewkey_export_submenu_values)) return NULL;
    if (N_oxen_state->viewkey_export_mode == idx)
        return viewkey_export_submenu_values_selected[idx];
    return viewkey_export_submenu_values[idx];
}

void viewkey_export_submenu_selector(unsigned int idx) {
    if (idx < ARRAYLEN(viewkey_export_submenu_values)) {
        unsigned char val = idx;
        nvm_write((void*) &N_oxen_state->viewkey_export_mode, &val, sizeof(unsigned char));
        monero_init();
    }
    ui_menu_settings_display_select(UI_SETTINGS_VIEW_KEY_EXPORT);
}

void ui_menu_viewkey_export_display() {
    ux_menulist_init_select(G_ux.stack_count - 1,
                            viewkey_export_submenu_getter,
                            viewkey_export_submenu_selector,
                            N_oxen_state->viewkey_export_mode);
}

/* -------------------------------- NETWORK UX --------------------------------- */

const char* const network_submenu_getter_values[] = {
#ifdef OXEN_ALPHA
    "Unvailable",
#else
    "Main Network",
#endif
    "Test Network",
    "Dev Network",
    "Cancel"};
const char* const network_submenu_getter_values_selected[] = {
#ifdef OXEN_ALPHA
    "Unvailable",
#else
    "Main Network *",
#endif
    "Test Network *",
    "Dev Network *",
    "Cancel"};

const char* network_submenu_getter(unsigned int idx) {
    if (idx >= ARRAYLEN(network_submenu_getter_values)) {
        return NULL;
    }
    int net;
    switch (idx) {
        case 0:
#ifdef OXEN_ALPHA
            net = -1;
#else
            net = MAINNET;
#endif
            break;
        case 1:
            net = TESTNET;
            break;
        case 2:
            net = DEVNET;
            break;
        default:
            net = -1;
            break;
    }
    if (N_oxen_state->network_id == net) {
        return network_submenu_getter_values_selected[idx];
    } else {
        return network_submenu_getter_values[idx];
    }
}

static void network_set_net(unsigned int network) {
    oxen_install(network);
    monero_init();
}

void network_submenu_selector(unsigned int idx) {
    switch (idx) {
        case 0:
#ifndef OXEN_ALPHA
            network_set_net(MAINNET);
#endif
            break;
        case 1:
            network_set_net(TESTNET);
            break;
        case 2:
            network_set_net(DEVNET);
            break;
        default:
            break;
    }
    ui_menu_main_display();
}

void ui_menu_network_display(void) {
    ux_menulist_init(G_ux.stack_count - 1, network_submenu_getter, network_submenu_selector);
}

/* -------------------------------- TRUNCATE ADDRS UX --------------------------------- */

const char* const truncate_addrs_values[] = {"Full address", "Short address", "Shorter addr"};
const char* const truncate_addrs_values_selected[] = {"Full address*",
                                                      "Short address*",
                                                      "Shorter addr*"};

const char* truncate_addrs_submenu_getter(unsigned int idx) {
    if (idx >= ARRAYLEN(truncate_addrs_values)) return NULL;
    if (N_oxen_state->truncate_addrs_mode == idx) return truncate_addrs_values_selected[idx];
    return truncate_addrs_values[idx];
}

void truncate_addrs_submenu_selector(unsigned int idx) {
    if (idx < ARRAYLEN(truncate_addrs_values)) {
        unsigned char val = idx;
        nvm_write((void*) &N_oxen_state->truncate_addrs_mode, &val, sizeof(unsigned char));
        monero_init();
    }
    ui_menu_settings_display_select(UI_SETTINGS_ADDRESS_CONFIRM);
}

void ui_menu_truncate_addrs_display() {
    ux_menulist_init_select(G_ux.stack_count - 1,
                            truncate_addrs_submenu_getter,
                            truncate_addrs_submenu_selector,
                            N_oxen_state->truncate_addrs_mode);
}

/* -------------------------------- CONFIRM FEE --------------------------------- */

const char* const confirm_fee_values[] = {"Always",
                                          "Above 0.05 OXEN",
                                          "Above 0.2 OXEN",
                                          "Above 1.0 OXEN"};
const char* const confirm_fee_values_selected[] = {"Always*",
                                                   "Above 0.05 OXEN*",
                                                   "Above 0.2 OXEN*",
                                                   "Above 1.0 OXEN*"};

const char* confirm_fee_submenu_getter(unsigned int idx) {
    if (idx >= ARRAYLEN(confirm_fee_values)) return NULL;
    if (N_oxen_state->confirm_fee_mode == idx) return confirm_fee_values_selected[idx];
    return confirm_fee_values[idx];
}

void confirm_fee_submenu_selector(unsigned int idx) {
    if (idx < ARRAYLEN(confirm_fee_values)) {
        unsigned char val = idx;
        nvm_write((void*) &N_oxen_state->confirm_fee_mode, &val, sizeof(unsigned char));
        monero_init();
    }
    ui_menu_settings_display_select(UI_SETTINGS_FEE_CONFIRM);
}

void ui_menu_confirm_fee_display() {
    ux_menulist_init_select(G_ux.stack_count - 1,
                            confirm_fee_submenu_getter,
                            confirm_fee_submenu_selector,
                            N_oxen_state->confirm_fee_mode);
}

/* -------------------------------- CONFIRM CHANGE --------------------------------- */

const char* const confirm_change_values[] = {"No", "Yes"};
const char* const confirm_change_values_selected[] = {"No*", "Yes*"};

const char* confirm_change_submenu_getter(unsigned int idx) {
    if (idx >= ARRAYLEN(confirm_change_values)) return NULL;
    if (N_oxen_state->confirm_change_mode == idx) return confirm_change_values_selected[idx];
    return confirm_change_values[idx];
}

void confirm_change_submenu_selector(unsigned int idx) {
    if (idx < ARRAYLEN(confirm_change_values)) {
        unsigned char val = idx;
        nvm_write((void*) &N_oxen_state->confirm_change_mode, &val, sizeof(unsigned char));
        monero_init();
    }
    ui_menu_settings_display_select(UI_SETTINGS_CHANGE_CONFIRM);
}

void ui_menu_confirm_change_display() {
    ux_menulist_init_select(G_ux.stack_count - 1,
                            confirm_change_submenu_getter,
                            confirm_change_submenu_selector,
                            N_oxen_state->confirm_change_mode);
}

/* -------------------------------- RESET UX --------------------------------- */
void ui_menu_reset_display(void);
void ui_menu_reset_action(unsigned int value);

UX_STEP_NOCB(ux_menu_reset_really_step, nn, {"", "Really Reset?"});
UX_STEP_CB(ux_menu_reset_no_step, pb, ui_menu_reset_action(REJECT), {&C_icon_crossmark, "No"});
UX_STEP_CB(ux_menu_reset_yes_step, pb, ui_menu_reset_action(ACCEPT), {&C_icon_validate_14, "Yes"});

UX_FLOW(ux_flow_reset, &ux_menu_reset_really_step, &ux_menu_reset_no_step, &ux_menu_reset_yes_step);

void ui_menu_reset_display(void) {
    ux_flow_init(0, ux_flow_reset, 0);
}

void ui_menu_reset_action(unsigned int value) {
    if (value == ACCEPT) {
        unsigned char magic[4] = {0};
        nvm_write((void*) N_oxen_state->magic, magic, 4);
        monero_init();
    }
    ui_menu_main_display();
}
/* ------------------------------- SETTINGS UX ------------------------------- */

const char* const settings_submenu_getter_values[] = {
    "<< Main menu",
    "View key export",
    "Fee confirm",
    "Address confirm",
    "Change confirm",
    "Select Network",
    "Reset",
    "Back",
};

const char* settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

void settings_submenu_selector(unsigned int idx) {
    switch (idx) {
        case UI_SETTINGS_VIEW_KEY_EXPORT:
            ui_menu_viewkey_export_display();
            break;
        case UI_SETTINGS_FEE_CONFIRM:
            ui_menu_confirm_fee_display();
            break;
        case UI_SETTINGS_ADDRESS_CONFIRM:
            ui_menu_truncate_addrs_display();
            break;
        case UI_SETTINGS_CHANGE_CONFIRM:
            ui_menu_confirm_change_display();
            break;
        case UI_SETTINGS_SELECT_NETWORK:
            ui_menu_network_display();
            break;
        case UI_SETTINGS_RESET:
            ui_menu_reset_display();
            break;
        default:
            ui_menu_main_display();  // UI_SETTINGS_BACK_TO_MAIN, UI_SETTINGS_BACK
    }
}

void ui_menu_settings_display_select(unsigned int idx) {
    ux_menulist_init_select(G_ux.stack_count - 1,
                            settings_submenu_getter,
                            settings_submenu_selector,
                            idx);
}
void ui_menu_settings_display(void) {
    ui_menu_settings_display_select(0);
}

/* ---------------------------- PUBLIC ADDRESS UX ---------------------------- */
void ui_menu_pubaddr_action(void);

UX_STEP_NOCB(ux_menu_pubaddr_meta_step,
             nn,
             {.line1 = G_oxen_state.ux_addr_type, .line2 = G_oxen_state.ux_addr_info});

UX_STEP_NOCB(ux_menu_pubaddr_address_step,
             bnnn_paging,
             {.title = "Address", .text = G_oxen_state.ux_address});

UX_STEP_CB(ux_menu_pubaddr_back_step,
           pb,
           ui_menu_pubaddr_action(),
           {.icon = &C_icon_back, .line1 = "Back"});

UX_FLOW(ux_flow_pubaddr,
        &ux_menu_pubaddr_meta_step,
        &ux_menu_pubaddr_address_step,
        &ux_menu_pubaddr_back_step,
        FLOW_LOOP);

void ui_menu_pubaddr_action(void) {
    if (G_oxen_state.disp_addr_mode) {
        monero_io_insert_u16(SW_OK);
        monero_io_do(IO_RETURN_AFTER_TX);
    }
    G_oxen_state.disp_addr_mode = 0;
    ui_menu_main_display();
}

/**
 *
 */
void ui_menu_any_pubaddr_display(unsigned char* pub_view,
                                 unsigned char* pub_spend,
                                 unsigned char is_subbadress,
                                 unsigned char* paymentID) {
    memset(G_oxen_state.ux_address, 0, sizeof(G_oxen_state.ux_address));
    memset(G_oxen_state.ux_addr_type, 0, sizeof(G_oxen_state.ux_addr_type));
    memset(G_oxen_state.ux_addr_info, 0, sizeof(G_oxen_state.ux_addr_info));

    switch (G_oxen_state.disp_addr_mode) {
        case 0:
        case DISP_MAIN:
            memmove(G_oxen_state.ux_addr_type, "Regular address", 15);
            if (N_oxen_state->network_id == MAINNET)
                memmove(G_oxen_state.ux_addr_info, "(mainnet)", 9);
            else if (N_oxen_state->network_id == TESTNET)
                memmove(G_oxen_state.ux_addr_info, "(testnet)", 9);
            else if (N_oxen_state->network_id == DEVNET)
                memmove(G_oxen_state.ux_addr_info, "(devnet)", 8);
            else if (N_oxen_state->network_id == FAKECHAIN)
                memmove(G_oxen_state.ux_addr_info, "(fakenet)", 9);
            break;

        case DISP_SUB:
            memmove(G_oxen_state.ux_addr_type, "Subaddress", 10);
            // Copy these out because they are in a union with the ux_addr_info string
            unsigned int M = G_oxen_state.disp_addr_M;
            unsigned int m = G_oxen_state.disp_addr_m;
            snprintf(G_oxen_state.ux_addr_info, 31, "Maj/min: %d/%d", M, m);
            break;

        case DISP_INTEGRATED:
            memmove(G_oxen_state.ux_addr_type, "Integr. address", 15);
            // Copy the payment id into place *first*, before the label, because it overlaps with
            // ux_addr_info
            memmove(G_oxen_state.ux_addr_info + 9, G_oxen_state.payment_id, 16);
            memmove(G_oxen_state.ux_addr_info, "Pay. ID: ", 9);
            break;
    }

    oxen_wallet_address(G_oxen_state.ux_address, pub_view, pub_spend, is_subbadress, paymentID);
    ux_flow_init(0, ux_flow_pubaddr, NULL);
}

void ui_menu_pubaddr_display(void) {
    G_oxen_state.disp_addr_mode = 0;
    G_oxen_state.disp_addr_M = 0;
    G_oxen_state.disp_addr_M = 0;
    ui_menu_any_pubaddr_display(G_oxen_state.view_pub, G_oxen_state.spend_pub, 0, NULL);
}

/* --------------------------------- MAIN UX --------------------------------- */

UX_STEP_CB(ux_menu_main_address_step,
           pnn,
           ui_menu_pubaddr_display(),
#ifndef TARGET_NANOS
           {&C_nanox_app_oxen, "OXEN wallet", G_oxen_state.ux_wallet_public_short_address});
#else
           {&C_nanos_app_oxen, "OXEN wallet", G_oxen_state.ux_wallet_public_short_address});
#endif

UX_STEP_CB(ux_menu_main_settings_step,
           pb,
           ui_menu_settings_display(),
           {&C_icon_coggle, "Settings"});

UX_STEP_NOCB(ux_menu_main_version_step, bn, {"Version", OXEN_VERSION_STRING});

UX_STEP_CB(ux_menu_main_quit_step, pb, os_sched_exit(0), {&C_icon_dashboard_x, "Quit app"});

UX_FLOW(ux_flow_main,
        &ux_menu_main_address_step,
        &ux_menu_main_settings_step,
        &ux_menu_main_version_step,
        &ux_menu_main_quit_step,
        FLOW_LOOP);

void ui_menu_main_display(void) {
    // reserve a display stack slot if none yet
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    ux_flow_init(0, ux_flow_main, NULL);
}
/* --- INIT --- */

void io_seproxyhal_display(const bagl_element_t* element) {
    io_seproxyhal_display_default((bagl_element_t*) element);
}
