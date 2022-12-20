//
// PCF8574 I2C GPIO Expander Handling
//
// © Copyright 2022 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <stdint.h>

extern int pcf8574_init(unsigned int ch);
extern int pcf8574_gpio_set(unsigned int ch, unsigned int offset, int state);
extern int pcf8574_gpio_get(unsigned int ch, unsigned int offset);
