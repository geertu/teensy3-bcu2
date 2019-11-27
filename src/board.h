//
// Board-Specific Configuration
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <stdint.h>

extern const uint8_t pin_heartbeat;

#define NUM_RGB_CH		2
#define NUM_POWER_CH		2
#define NUM_KEY_CH		6
#define NUM_GPIO_CH		2
#define NUM_UART_CH		2

extern const uint8_t pin_rgb[];
extern const uint8_t pin_power[];
extern const uint8_t pin_key[];
extern const uint8_t pin_gpio[];
