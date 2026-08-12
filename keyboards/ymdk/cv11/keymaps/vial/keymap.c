// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "raw_hid.h"

enum layers {
    _BASE, // normal keyboard: volume knob + digits
    _RAW,  // keys + encoder reported to the host as raw HID events
};

/* ------------------------------------------------------------------ *
 *  Raw-HID protocol (shared with the host application)
 *
 *  Device -> host (unsolicited, on the VIA/raw-HID interface). The host
 *  filters incoming 32-byte reports on byte [0]:
 *      [0]=0x01 RH_ENC_ROTATE : [1]=encoder index, [2]=1 CW / 0 CCW
 *      [0]=0x03 RH_SLOT_PRESS : [1]=slot (0..10),  [2]=1 pressed / 0 released
 *          slot 0 = encoder button, slot 1 = top-right, slots 2..10 = 3x3 grid
 *
 *  Host -> device (VIA forwards unknown command id 0xC0 to raw_hid_receive_kb):
 *      [0]=0xC0 [1]=0x01 RH_SET_LAYER [2]=layer   -> switch active layer
 *      [0]=0xC0 [1]=0x02 RH_GET_LAYER            -> echoes [.. ,[2]=current layer]
 * ------------------------------------------------------------------ */
enum { RH_ENC_ROTATE = 0x01, RH_SLOT_PRESS = 0x03 };
#define RH_APP_CMD 0xC0
enum { RH_SET_LAYER = 0x01, RH_GET_LAYER = 0x02 };
#define RH_REPORT_SIZE 32

static void rh_send(uint8_t type, uint8_t a, uint8_t b) {
    uint8_t report[RH_REPORT_SIZE] = {0};
    report[0] = type;
    report[1] = a;
    report[2] = b;
    raw_hid_send(report, sizeof(report));
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_MUTE,          KC_ESC,
        KC_1,    KC_2,    KC_3,
        KC_4,    KC_5,    KC_6,
        KC_7,    KC_8,    KC_9
    ),
    /* keycodes here are unused: on _RAW every key is intercepted and sent as a
       slot event. Left as KC_NO so nothing leaks if interception is disabled. */
    [_RAW] = LAYOUT(
        KC_NO,            KC_NO,
        KC_NO,   KC_NO,   KC_NO,
        KC_NO,   KC_NO,   KC_NO,
        KC_NO,   KC_NO,   KC_NO
    ),
    [2] = LAYOUT(
        _______,          _______,
        _______, _______, _______,
        _______, _______, _______,
        _______, _______, _______
    ),
    [3] = LAYOUT(
        _______,          _______,
        _______, _______, _______,
        _______, _______, _______,
        _______, _______, _______
    )
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_RAW]  = { ENCODER_CCW_CW(KC_NO,   KC_NO)   },
    [2]     = { ENCODER_CCW_CW(_______, _______) },
    [3]     = { ENCODER_CCW_CW(_______, _______) }
};
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (get_highest_layer(layer_state) != _RAW)
        return true; // _BASE etc: normal keyboard, encoder map drives volume

    // On _RAW, every event is reported to the host. With ENCODER_MAP the encoder
    // arrives here too, as an ENCODER_{CW,CCW}_EVENT (its .col is the encoder
    // index, so the direction lives in the event type, not the column).
    keyevent_t *e = &record->event;
    if (e->type == ENCODER_CW_EVENT || e->type == ENCODER_CCW_EVENT) {
        if (e->pressed) // map fires press+release per detent; report once
            rh_send(RH_ENC_ROTATE, e->key.col, e->type == ENCODER_CW_EVENT ? 1 : 0);
    } else {
        // real key: matrix is 1x11, so the column is the slot (0 = encoder button)
        rh_send(RH_SLOT_PRESS, e->key.col, e->pressed ? 1 : 0);
    }
    return false; // consume everything while on the raw layer
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
