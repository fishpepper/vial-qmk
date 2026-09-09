// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/* U1 is a W25Q64: 64 Mbit / 8 MB. The pico-sdk default is 2 MB (boards/pico.h),
 * which leaves most of the chip unused and puts the wear-levelling EEPROM
 * backing store somewhere other than the end of flash.
 *
 * No stage-2 override needed: QMK's default boot2 (W25Q080) drives the whole
 * Winbond W25Q family the same way, so XIP runs quad-SPI at clk_sys/4 with the
 * stock 125 MHz system clock. */
#define PICO_FLASH_SIZE_BYTES (8 * 1024 * 1024)

/* Double tap reset to enter the bootloader without holding BOOT while
 * plugging in. GP9 drives the red status LED (active high), which the bootrom
 * then uses as the UF2 activity indicator. */
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP9
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_TIMEOUT 500U
