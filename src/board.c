//
// Board-Specific Configuration
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include "board.h"

const uint8_t pin_heartbeat = 13;

const uint8_t pin_rgb[NUM_RGB_CH * 3] = {
    5, 4, 3,
    22, 21, 20
};

const uint8_t pin_power[NUM_POWER_CH] = { 7, 17 };

const uint8_t pin_key[NUM_KEY_CH] = { 8, 11, 12, 14, 15, 16 };

const uint8_t pin_gpio[NUM_GPIO_CH] = { 2, 23 };
