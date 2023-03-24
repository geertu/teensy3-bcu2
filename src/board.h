//
// Board-Specific Configuration
//
// © Copyright 2019-2020, 2022 Glider bv
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
extern const uint8_t ina219_map[];

extern void power_init(void);
extern void power_set(unsigned int ch, int on);
extern int power_get(unsigned int ch);

extern void key_init(void);
extern void key_set(unsigned int ch, int on);
extern int key_get(unsigned int ch);

extern void gpio_init(void);
extern void gpio_set(unsigned int ch, int on);
extern int gpio_get(unsigned int ch);
