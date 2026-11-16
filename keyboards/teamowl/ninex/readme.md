# ninex

A 12-key (4x3) RP2040 macropad with per-key RGB, derived from the Adafruit
MacroPad RP2040 design (same MCU section, no encoder / OLED / speaker).

* Keyboard Maintainer: [fishpepper](https://github.com/fishpepper)
* Hardware Supported: ninex PCB (`ninex_2` revision), RP2040 QFN-56 +
  W25Q64 flash, 12MHz crystal
* Hardware Availability: <https://github.com/fishpepper/ninex>

## Hardware

| Function            | Pin(s)                                          |
| ------------------- | ----------------------------------------------- |
| Switches (direct)   | row 0: GP28, GP29, GP27, GP0                    |
|                     | row 1: GP25, GP26, GP21, GP5                    |
|                     | row 2: GP20, GP18, GP17, GP16                   |
| WS2812 data         | GP19 (via 74HCT1G125 level shifter, LEDs on 5V) |
| Status LED (red)    | GP9, active high                                |
| BOOT button         | QSPI_CS (bootrom BOOTSEL, not a matrix key)     |

Each switch is wired directly between its GPIO and GND — there are no diodes
and no scanning matrix, so the "matrix" is a 3x4 direct-pin map that mirrors
the physical key grid. The 12 WS2812 LEDs are chained in a serpentine starting
at the top left key: row 0 left-to-right, row 1 right-to-left, row 2
left-to-right; `rgb_matrix.layout` in `keyboard.json` follows that order.

Key pitch is 19.5 mm in both directions, except one column, which sits 25 mm
from its neighbour — that is why the layouts use a 0.28u gap there.

The matrix is numbered in physical reading order, with the pad in its normal
orientation (USB cable going away from you, wide column gap on the right):
row 0 is the row nearest the USB connector, column 0 the leftmost one, so
`[0, 0]` is the top-left key and `[2, 3]` the bottom-right one. Keymaps are
written in that same reading order. Bootmagic is on `[2, 3]`, the bottom-right
key.

The pin map above comes straight from the KiCad schematic; `kicad-cli sch
export netlist` on `ninex.kicad_sch` plus the socket placements in
`ninex.kicad_pcb` is how to re-derive it after a board revision.

## Bring-up aids

Two things in `ninex.c` are there for bring-up and can be deleted once the
board is trusted:

* solid green on every boot via `keyboard_post_init_kb()`, so a missing or
  miswired LED shows up immediately. Nothing is written to EEPROM, so Vial
  changes still apply live but do not survive a replug.
* a heartbeat on the red status LED: solid from early boot, 1 Hz blink once
  the main loop is running.

`rules.mk` also pins `BOARD = GENERIC_RP_RP2040`. QMK's default board for
RP2040 is `GENERIC_PROMICRO_RP2040`, whose config assigns Pro Micro peripheral
pins this board does not have — including `SPI_MOSI_PIN GP19`, which is the
WS2812 data line here.

There is a `ninex-ram-boot` branch that builds the firmware to run entirely
from SRAM (`make teamowl/ninex:vial NINEX_RAM=yes`, then
`picotool load -x`), for a board whose QSPI flash is missing or dead.

Make example for this keyboard (after setting up your build environment):

    make teamowl/ninex:vial

Flashing example for this keyboard:

    make teamowl/ninex:vial:flash

## Bootloader

Enter the bootloader in one of three ways:

* **Bootmagic reset**: hold the bottom-right key and plug in the keyboard
* **BOOT button**: hold BOOT while plugging in the board
* **Double tap reset**: tap reset twice (`RP2040_BOOTLOADER_DOUBLE_TAP_RESET`)
* **Keycode**: `QK_BOOT` — on layer 2, the top-left key in the shipped keymap
