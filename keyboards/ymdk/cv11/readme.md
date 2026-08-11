# YMDK CV11 (Vial)

*Cheap and awesome 11 keys keypad, commonly called "CV11". It has 10 keys + 1 Rotary Encoder, and 11 RGB LEDs.*

* Keyboard Maintainer: QMK Community
* Hardware Supported: STM32F103xB (128KB) - See [here](https://github.com/agordon/qmk-vial-ymdk-cv11) for hardware details.
* Hardware Availability: Search for "CV11" on most retail websites.

## Build

    qmk compile -kb ymdk/cv11 -km vial

This creates `ymdk_cv11_vial.uf2`. A plain (non-Vial) build is also available:

    qmk compile -kb ymdk/cv11 -km default

## Vial

This keymap is [Vial](https://get.vial.today/)-enabled with per-key RGB
(VialRGB). To unlock Vial (required before making changes to some settings),
hold the **encoder button + the bottom-right key**.

## Bootloader

This board has **no dedicated hardware bootloader button**, and the single
physical `RST` button only resets into the currently installed firmware. The
DFU/UF2 bootloader can therefore only be entered *from the running firmware*:

* **Bootmagic reset**: unplug the keyboard, press and hold the rotary encoder
  button, plug it back in while still holding, and keep holding for a couple of
  seconds. It re-enumerates as a USB mass-storage drive named `MT.KEY`.
* **`QK_BOOT` keycode**: press a key mapped (e.g. via Vial) to `QK_BOOT`.

To flash, copy the `.uf2` file onto the `MT.KEY` drive.

> **Warning:** because bootloader entry depends on the running firmware,
> flashing firmware that hangs before bootmagic runs can lock you out of the
> bootloader. Recovery then requires an SWD/ST-Link programmer.
