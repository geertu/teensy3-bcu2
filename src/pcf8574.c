//
// PCF8574 I2C GPIO Expander Handling
//
// © Copyright 2022 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#ifdef POWER_OPTO_EXTENSION

#include <twi.h>

#include "pcf8574.h"
#include "print.h"
#include "util.h"

// PCF8574 I2C address base
#define PCF8574_BASE		      0x20 // Up to 4 devices */

static uint8_t pcf8574_cache[4];

static int pcf8574_write(unsigned int ch, uint8_t val)
{
	int res;

	res = twi_writeTo(PCF8574_BASE + ch, &val, 1, true, true);
	if (res)
		pr_err("%s: twi_writeTo() returned error %d\n", __func__, res);
	else
		pcf8574_cache[ch] = val;

	return -res;
}

static int pcf8574_read(unsigned int ch)
{
	uint8_t val;
	int res;

	res = twi_readFrom(PCF8574_BASE + ch, &val, 1, true);
	if (res != 1) {
		pr_err("%s: twi_readFrom() read only %d bytes\n", __func__,
		       res);
		return -1;
	}

	return val;
}

int pcf8574_init(unsigned int ch)
{
	return pcf8574_write(ch, 0xff);
}

//
// State must be 1 for high or input (weak pull-up), 0 for low output.
//
int pcf8574_gpio_set(unsigned int ch, unsigned int offset, int state)
{
	uint8_t val = pcf8574_cache[ch];

	if (state)
		val |= BIT(offset);
	else
		val &= ~BIT(offset);

	return pcf8574_write(ch, val);
}

int pcf8574_gpio_get(unsigned int ch, unsigned int offset)
{
	uint8_t val = pcf8574_read(ch);

	return !!(val & BIT(offset));
}

#endif /* POWER_OPTO_EXTENSION */
