# Not GENERIC_PROMICRO_RP2040, which is QMK's default board for RP2040: that
# config hands out Pro Micro pin defaults this board does not have (its
# SPI_MOSI_PIN GP19 is the WS2812 data line here) and force-defines
# RP2040_BOOTLOADER_DOUBLE_TAP_RESET, overriding whatever config.h says.
BOARD = GENERIC_RP_RP2040

# Match the linker's view of XIP to the actual W25Q64 (64 Mbit / 8 MB); the
# RP2040 linker scripts otherwise assume 2 MB. See PICO_FLASH_SIZE_BYTES in
# config.h, which is what places the EEPROM backing store.
LDFLAGS += -Wl,--defsym=FLASH_LEN=8388608
