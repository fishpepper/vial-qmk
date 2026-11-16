// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"

/* Heartbeat on the red status LED (GP9, active high): solid from early boot,
 * blinking at 1 Hz once the main loop is running. */
void keyboard_pre_init_kb(void) {
    gpio_set_pin_output(GP9);
    gpio_write_pin_high(GP9);

    keyboard_pre_init_user();
}

void housekeeping_task_kb(void) {
    static uint32_t last_toggle = 0;
    static bool     lit         = true;

    if (timer_elapsed32(last_toggle) > 500) {
        last_toggle = timer_read32();
        lit         = !lit;
        gpio_write_pin(GP9, lit);
    }

    housekeeping_task_user();
}
