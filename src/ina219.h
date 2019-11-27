//
// INA219 Power Monitor Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <stdint.h>

extern int ina219_init(unsigned int ch);
extern int ina219_get_shunt_uV(unsigned int ch);
extern int ina219_get_bus_mV(unsigned int ch);
extern int ina219_get_power_mW(unsigned int ch);
extern int ina219_get_current_mA(unsigned int ch);
