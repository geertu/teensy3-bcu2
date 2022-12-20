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
#include "pcf8574.h"
#include "util.h"

const uint8_t pin_heartbeat = 13;

const uint8_t pin_rgb[NUM_RGB_CH * 3] = {
    5, 4, 3,
    22, 21, 20
};

const uint8_t ina219_map[] = { 0, 1, 4, 5 };

static const uint8_t pin_power[] = { 7, 17 };

static const uint8_t pin_key[] = { 8, 11, 12, 14, 15, 16 };

static const uint8_t pin_gpio[NUM_GPIO_CH] = { 2, 23 };

static char power_cache[NUM_POWER_CH];
static char key_cache[NUM_KEY_CH];
static char gpio_cache[NUM_GPIO_CH];

void power_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pin_power); i++) {
		pinMode(pin_power[i], OUTPUT);
		digitalWrite(pin_power[i], 0);
	}
}

void power_set(unsigned int ch, int on)
{
#ifdef POWER_OPTO_EXTENSION
	if (ch >= ARRAY_SIZE(pin_power)) {
		/* Active-low! */
		pcf8574_gpio_set(0, ch == ARRAY_SIZE(pin_power) ? 0 : 7, !on);
	} else
#endif // POWER_OPTO_EXTENSION
	{
		digitalWrite(pin_power[ch], on);
	}

	power_cache[ch] = on;
}

int power_get(unsigned int ch)
{
	return power_cache[ch];
}

void key_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pin_key); i++) {
		pinMode(pin_key[i], OUTPUT);
		digitalWrite(pin_key[i], 1);
	}
}

void key_set(unsigned int ch, int on)
{
#ifdef POWER_OPTO_EXTENSION
	if (ch >= ARRAY_SIZE(pin_key)) {
		/* Active-low! */
		pcf8574_gpio_set(0, ch - ARRAY_SIZE(pin_key) + 1, !on);
	} else
#endif // POWER_OPTO_EXTENSION
	{
		/* Active-low! */
		digitalWrite(pin_key[ch], !on);
	}

	key_cache[ch] = on;
}

int key_get(unsigned int ch)
{
	return key_cache[ch];
}

void gpio_init(void)
{
	unsigned int i;

	for (i = 0; i < NUM_GPIO_CH; i++) {
		pinMode(pin_gpio[i], OUTPUT);
		digitalWrite(pin_gpio[i], 0);
	}
}

void gpio_set(unsigned int ch, int on)
{
	digitalWrite(pin_gpio[ch], !on);
	gpio_cache[ch] = on;
}

int gpio_get(unsigned int ch)
{
	return gpio_cache[ch];
}
