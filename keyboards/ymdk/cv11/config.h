// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// The STM32F103xB on this board reports 64KB in its flash-size register even
// though it physically has 128KB. Without this override the wear_leveling_efl
// EEPROM backing store is placed inside the application code and HardFaults on
// first write (which VIA/VIAL do constantly).
#define WEAR_LEVELING_EFL_FLASH_SIZE (128 * 1024)

// WS2812 PWM driver via TIM4_CH4 (PB9) + DMA1_CH7 (TIM4_UP)
#define WS2812_PWM_DRIVER       PWMD4
#define WS2812_PWM_CHANNEL      4
#define WS2812_PWM_DMA_STREAM   STM32_DMA1_STREAM7
#define WS2812_PWM_DMA_CHANNEL  7
