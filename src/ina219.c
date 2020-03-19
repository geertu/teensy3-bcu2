//
// INA219 Power Monitor Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <twi.h>

#include "ina219.h"
#include "util.h"
#include "print.h"

// INA219 I2C address base
#define INA219_BASE		      0x40 // Up to 4 devices */

#define INA219_CFG		      0x00 // Configuration
#define INA219_SHUNT_V		      0x01 // Shunt Voltage
#define INA219_BUS_V		      0x02 // Bus Voltage
#define INA219_POWER		      0x03 // Power
#define INA219_CURRENT		      0x04 // Current
#define INA219_CALIB		      0x05 // Calibration

// INA219 Config Register Bit Definitions
#define INA219_CFG_RST		   BIT(15) // Reset
#define INA219_CFG_BRNG		   BIT(13) // Bus Voltage Range
#define INA219_CFG_BRNG_16V		 0 // 16V
#define INA219_CFG_BRNG_32V	   BIT(13) // 32V
#define INA219_CFG_GAIN_MASK	 (3 << 12) // PGA Gain and Range
#define INA219_CFG_GAIN_1	 (0 << 12) // ±40 mV
#define INA219_CFG_GAIN_2	 (1 << 12) // ±80 mV
#define INA219_CFG_GAIN_4	 (2 << 12) // ±160 mV
#define INA219_CFG_GAIN_8	 (3 << 12) // ±320 mV
#define INA219_CFG_BADC_MASK	    0x0780 // Bus ADC Resolution/Averaging
#define INA219_CFG_BADC_SHIFT		 7
#define INA219_CFG_SADC_MASK	    0x0078 // Shunt ADC Resolution/Averaging
#define INA219_CFG_SADC_SHIFT		 3

#define INA219_CFG_xADC_9B		 0 // 9 bit
#define INA219_CFG_xADC_10B		 1 // 10 bit
#define INA219_CFG_xADC_11B		 2 // 11 bit
#define INA219_CFG_xADC_12B		 3 // 12 bit
#define INA219_CFG_xADC_2S		 9 // 2 samples
#define INA219_CFG_xADC_4S		10 // 4 samples
#define INA219_CFG_xADC_8S		11 // 8 samples
#define INA219_CFG_xADC_16S		12 // 16 samples
#define INA219_CFG_xADC_32S		13 // 32 samples
#define INA219_CFG_xADC_64S		14 // 64 samples
#define INA219_CFG_xADC_128S		15 // 128 samples

#define INA219_CFG_MODE_MASK	    0x0007 // Operating Mode
#define INA219_CFG_MODE_POWER_DOWN	 0 // Power-Down
#define INA219_CFG_MODE_SHUNT_TRG	 1 // Shunt voltage, triggered
#define INA219_CFG_MODE_BUS_TRG		 2 // Bus voltage, triggered
#define INA219_CFG_MODE_SHUNT_BUS_TRG	 3 // Shunt and bus voltage, triggered
#define INA219_CFG_MODE_ADC_OFF		 4 // ADC off (disabled)
#define INA219_CFG_MODE_SHUNT_CNT	 5 // Shunt voltage, continuous
#define INA219_CFG_MODE_BUS_CNT		 6 // Bus voltage, continuous
#define INA219_CFG_MODE_SHUNT_BUS_CNT	 7 // Shunt and bus voltage, continuous

// INA219 Bus Voltage Register Bit Definitions
#define INA219_BUS_V_CNVR	     BIT(1) // Conversion Ready
#define INA219_BUS_V_OVF	     BIT(0) // Math Overflow Flag


static uint16_t ina219_config;

// Defaults for the Adafruit INA219 Current Sensor Breakout
static const unsigned int ina219_calib = 4096;
static const unsigned int ina219_current_div_mA = 10;
static const unsigned int ina219_power_mult_mW = 2;


static int ina219_write(unsigned int ch, uint8_t reg, uint16_t val)
{
	uint8_t buf[3];
	int res;

	buf[0] = reg;
	buf[1] = val >> 8;
	buf[2] = val;
	res = twi_writeTo(INA219_BASE + ch, buf, sizeof(buf), true, true);
	pr_debug("twi_writeTo() returned %u\n", res);
	if (res)
		pr_err("%s: twi_writeTo() returned error %d\n", __func__, res);

	return -res;
}

static int ina219_read(unsigned int ch, uint8_t reg)
{
	uint8_t buf[2];
	int res;

	res = twi_writeTo(INA219_BASE + ch, &reg, 1, true, true);
	pr_debug("twi_writeTo() returned %u\n", res);
	if (res) {
		pr_err("%s: twi_writeTo() returned error %d\n", __func__, res);
		return -res;
	}

	res = twi_readFrom(INA219_BASE + ch, buf, sizeof(buf), true);
	pr_debug("twi_readFrom() returned %u\n", res);
	if (res != sizeof(buf)) {
		pr_err("%s: twi_readFrom() read only %d bytes\n", __func__,
		       res);
		return -1;
	}

	return buf[0] << 8 | buf[1];
}

static void ina219_dump_config(uint16_t cfg)
{
	uint8_t brng = 0, gain = 0, bus_bits = 0, shunt_bits = 0;
	const char *mode;

	switch (cfg & INA219_CFG_BRNG) {
	case INA219_CFG_BRNG_16V:
		brng = 16;
		break;

	case INA219_CFG_BRNG_32V:
		brng = 32;
		break;
	}

	switch (cfg & INA219_CFG_GAIN_MASK) {
	case INA219_CFG_GAIN_1:
		gain = 1;
		break;

	case INA219_CFG_GAIN_2:
		gain = 2;
		break;

	case INA219_CFG_GAIN_4:
		gain = 4;
		break;

	case INA219_CFG_GAIN_8:
		gain = 8;
		break;

	}

	switch ((cfg & INA219_CFG_BADC_MASK) >> INA219_CFG_BADC_SHIFT) {
	case INA219_CFG_xADC_9B:
		bus_bits = 9;
		break;

	case INA219_CFG_xADC_10B:
		bus_bits = 10;
		break;

	case INA219_CFG_xADC_11B:
		bus_bits = 11;
		break;

	case INA219_CFG_xADC_12B:
		bus_bits = 12;
		break;
	}

	switch ((cfg & INA219_CFG_SADC_MASK) >> INA219_CFG_SADC_SHIFT) {
	case INA219_CFG_xADC_9B:
		shunt_bits = 9;
		break;

	case INA219_CFG_xADC_10B:
		shunt_bits = 10;
		break;

	case INA219_CFG_xADC_11B:
		shunt_bits = 11;
		break;

	case INA219_CFG_xADC_12B:
		shunt_bits = 12;
		break;
	}

	switch ((cfg & INA219_CFG_MODE_MASK)) {
	case INA219_CFG_MODE_POWER_DOWN:
		mode = "Power-Down";
		break;
	case INA219_CFG_MODE_SHUNT_TRG:
		mode = "Shunt voltage, triggered";
		break;
	case INA219_CFG_MODE_BUS_TRG:
		mode = "Bus voltage, triggered";
		break;
	case INA219_CFG_MODE_SHUNT_BUS_TRG:
		mode = "Shunt and bus voltage, triggered";
		break;
	case INA219_CFG_MODE_ADC_OFF:
		mode = "ADC off (disabled)";
		break;
	case INA219_CFG_MODE_SHUNT_CNT:
		mode = "Shunt voltage, continuous";
		break;
	case INA219_CFG_MODE_BUS_CNT:
		mode = "Bus voltage, continuous";
		break;
	case INA219_CFG_MODE_SHUNT_BUS_CNT:
		mode = "Shunt and bus voltage, continuous";
		break;
	}

	pr_info("INA219_CFG           = 0x%x\n", cfg);
	pr_info("Bus Voltage Range    = %2u\n", brng);
	pr_info("Shunt Voltage Gain   = %2u\n", gain);
	pr_info("Bus ADC resolution   = %2u\n", bus_bits);
	pr_info("Shunt ADC resolution = %2u\n", shunt_bits);
	pr_info("Operating mode       = %s\n", mode);
}

int ina219_init(unsigned int ch)
{
	int x;

	// In order for the device to report both current and power values, the
	// user must program the resolution of the Current Register (04h) and
	// the value of the shunt resistor (R SHUNT ) present in the
	// application to develop the differential voltage applied between the
	// input pins. Both the Current_LSB and shunt resistor value are used
	// in the calculation of the Calibration Register value that the device
	// uses to calculate the corresponding current and power values based
	// on the measured shunt and bus voltages.
	//
	// After programming the Calibration Register, the Current Register
	// (04h) and Power Register (03h) update accordingly based on the
	// corresponding shunt voltage and bus voltage measurements. Until the
	// Calibration Register is programmed, the Current Register (04h) and
	// Power Register (03h) remain at zero.
	//
	// The INA219 can be used without any programming if it is only
	// necessary to read a shunt voltage drop and bus voltage with the
	// default 12-bit resolution, 320-mV shunt full-scale range (PGA = /8),
	// 32-V bus full-scale range, and continuous conversion of shunt and
	// bus voltage.
	//
	// Without programming, current is measured by reading the shunt
	// voltage.  The Current register and Power register are only available
	// if the Calibration register contains a programmed value.

	x = ina219_read(ch, INA219_CFG);
	if (x < 0)
		return x;

	if (!ch) {
		ina219_config = x;
		ina219_dump_config(x);
	} else if (x != ina219_config) {
		pr_err("INA219_CFG mismatch: ch0 0x%x ch1 0x%x\n",
		       ina219_config, x);
		ina219_dump_config(x);
	}

	x = ina219_read(ch, INA219_CALIB);
	if (x < 0)
		return x;

	pr_debug("INA219_CALIB         = 0x%04x/%u\n", x, x);

	ina219_write(ch, INA219_CALIB, ina219_calib);

	x = ina219_read(ch, INA219_CALIB);
	if (x < 0)
		return x;

	pr_debug("INA219_CALIB         = 0x%04x/%u (NEW)\n", x, x);

	return 0;
}

int ina219_get_shunt_uV(unsigned int ch)
{
	int x = ina219_read(ch, INA219_SHUNT_V);
	pr_debug("INA219_SHUNT_V = 0x%04x\n", x);
	if (x < 0)
		return x;

	return x * 10;
}

int ina219_get_bus_mV(unsigned int ch)
{
	int x;

	do {
		x = ina219_read(ch, INA219_BUS_V);
		pr_debug("INA219_BUS_V   = 0x%04x\n", x);
		if (x < 0)
			return x;
	} while (!(x & INA219_BUS_V_CNVR));

	if (x & INA219_BUS_V_OVF) {
		pr_err("INA219 Math Overflow!\n");
	}

	return 4 * (x >> 3);
}

int ina219_get_power_mW(unsigned int ch)
{
	int x = ina219_read(ch, INA219_POWER);
	pr_debug("INA219_POWER   = 0x%04x\n", x);
	if (x < 0)
		return x;

	return x * ina219_power_mult_mW;
}

int ina219_get_current_mA(unsigned int ch)
{
	int x = ina219_read(ch, INA219_CURRENT);
	pr_debug("INA219_CURRENT = 0x%04x\n", x);
	if (x < 0)
		return x;

	return DIV_ROUND_CLOSEST(x, ina219_current_div_mA);
}
