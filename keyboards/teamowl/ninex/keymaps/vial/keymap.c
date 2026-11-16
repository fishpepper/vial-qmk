// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "raw_hid.h"

enum layers {
    _BASE, // normal keyboard: digits, enter, up/down
    _RAW,  // keys reported to the host as raw HID events
};

/* ------------------------------------------------------------------ *
 *  Raw-HID protocol (shared with the host application)
 *
 *  Device -> host (unsolicited, on the VIA/raw-HID interface). The host
 *  filters incoming 32-byte reports on byte [0]:
 *      [0]=0x03 RH_SLOT_PRESS : [1]=slot (0..11), [2]=1 pressed / 0 released
 *          slots run in physical reading order, 0 = top-left, 11 = bottom-
 *          right, which is also the matrix order - see ninex_slot().
 *
 *  Host -> device (VIA forwards unknown command id 0xC0 to raw_hid_receive_kb):
 *      [0]=0xC0 [1]=0x01 RH_SET_LAYER [2]=layer   -> switch active layer
 *      [0]=0xC0 [1]=0x02 RH_GET_LAYER            -> echoes [.. ,[2]=current layer]
 *
 *  Same wire format as the ymdk/cv11 pad; RH_ENC_ROTATE (0x01) is unused here
 *  because this board has no encoder.
 * ------------------------------------------------------------------ */
enum { RH_SLOT_PRESS = 0x03 };
#define RH_APP_CMD 0xC0
enum { RH_SET_LAYER = 0x01, RH_GET_LAYER = 0x02 };
#define RH_REPORT_SIZE 32

/* Raw-HID reports are vendor traffic: the OS does not count them as user
 * input, so while the host app owns the pad the machine looks idle and Teams
 * (and screen lock, and presence in general) decides you are away. Tap a
 * keycode on the normal keyboard interface alongside each slot press purely so
 * the idle timer gets poked.
 *
 * A lone modifier is the most inert thing that still counts: it produces no
 * character and no menu activation (unlike Alt or GUI). Override in config.h
 * if it ever collides with something. */
#ifndef NINEX_RAW_ACTIVITY_KEYCODE
#    define NINEX_RAW_ACTIVITY_KEYCODE KC_RIGHT_CTRL
#endif

static void rh_send(uint8_t type, uint8_t a, uint8_t b) {
    uint8_t report[RH_REPORT_SIZE] = {0};
    report[0] = type;
    report[1] = a;
    report[2] = b;
    raw_hid_send(report, sizeof(report));
}

/* Matrix position -> physical slot. The matrix is numbered in physical
 * reading order (row 0 = the row nearest the USB connector, column 0 =
 * leftmost), so the slot index is just the position in that grid. */
static uint8_t ninex_slot(uint8_t row, uint8_t col) {
    return (uint8_t)(row * MATRIX_COLS + col);
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_1, KC_2, KC_3, KC_ENT,
        KC_4, KC_5, KC_6, KC_UP,
        KC_7, KC_8, KC_9, KC_DOWN
    ),
    /* keycodes here are unused: on _RAW every key is intercepted and sent as a
       slot event. Left as KC_NO so nothing leaks if interception is disabled. */
    [_RAW] = LAYOUT(
        KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO,
        KC_NO, KC_NO, KC_NO, KC_NO
    ),
    /* QK_BOOT lives here rather than on _RAW, which swallows every key. Reach
       it by assigning a layer key in Vial, or via RH_SET_LAYER from the host.
       Bootmagic (hold the bottom-right key while plugging in) and the BOOT
       button remain the routes that need no working firmware. */
    [2] = LAYOUT(
        QK_BOOT, _______, _______, _______,
        _______, _______, _______, _______,
        _______, _______, _______, _______
    ),
    [3] = LAYOUT(
        _______, _______, _______, _______,
        _______, _______, _______, _______,
        _______, _______, _______, _______
    )
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (get_highest_layer(layer_state) != _RAW)
        return true; // _BASE etc: normal keyboard

    keyevent_t *e = &record->event;
    rh_send(RH_SLOT_PRESS, ninex_slot(e->key.row, e->key.col), e->pressed ? 1 : 0);

    // Keep the host's idle timer alive - see NINEX_RAW_ACTIVITY_KEYCODE.
    // On press only: tapping is enough to register activity, and mirroring the
    // hold would leave a modifier stuck down for as long as the key is held.
    if (e->pressed) tap_code(NINEX_RAW_ACTIVITY_KEYCODE);

    return false; // the keycodes themselves stay consumed on the raw layer
}

// Host -> device commands that VIA/Vial does not recognise are routed here.
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != RH_APP_CMD) return;
    switch (data[1]) {
        case RH_SET_LAYER:
            if (length >= 3) layer_move(data[2]);
            break;
        case RH_GET_LAYER:
            data[2] = get_highest_layer(layer_state);
            break;
    }
    // VIA echoes `data` back to the host after this returns.
}
