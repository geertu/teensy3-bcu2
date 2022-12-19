//
// Board-Specific Configuration
//
// © Copyright 2019-2020, 2022 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <WProgram.h>

#include "board.h"

const uint8_t pin_heartbeat = 13;

const uint8_t pin_rgb[NUM_RGB_CH * 3] = {
    5, 4, 3,
    22, 21, 20
};

static const uint8_t pin_power[NUM_POWER_CH] = { 7, 17 };

static char power_cache[NUM_POWER_CH];

void power_init(void)
{
	unsigned int i;

	for (i = 0; i < NUM_POWER_CH; i++) {
		pinMode(pin_power[i], OUTPUT);
		digitalWrite(pin_power[i], 0);
	}
}

void power_set(unsigned int ch, int on)
{
	digitalWrite(pin_power[ch], on);
	power_cache[ch] = on;
}

int power_get(unsigned int ch)
{
	return power_cache[ch];
}

const uint8_t pin_key[NUM_KEY_CH] = { 8, 11, 12, 14, 15, 16 };

const uint8_t pin_gpio[NUM_GPIO_CH] = { 2, 23 };
