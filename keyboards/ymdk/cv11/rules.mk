# This is the 128KB STM32F103xB variant; its flash-size register under-reports
# as 64KB, so select the 128KB linker script explicitly. Combined with the
# uf2boot bootloader this links the application at 0x08004000 (16KB reserved).
MCU_LDSCRIPT = STM32F103xB

# Persist VIA/VIAL dynamic keymaps in emulated flash EEPROM.
EEPROM_DRIVER = wear_leveling
WEAR_LEVELING_DRIVER = embedded_flash
