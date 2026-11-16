// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

/* Keys are listed in physical reading order, which is also the matrix order:
 * row 0 is the row nearest the USB connector, column 0 the leftmost one. */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT(
      KC_1, KC_2, KC_3, KC_ENT,
      KC_4, KC_5, KC_6, KC_UP,
      KC_7, KC_8, KC_9, KC_DOWN
  )
};
