#include "twi.h"
#include "i2c_t3.h"

void twi_init(void)
{
	Wire.begin();
}

uint8_t twi_readFrom(uint8_t address, uint8_t *data, uint8_t length,
		     uint8_t sendStop)
{
	unsigned int i;
	size_t n;

	n = Wire.requestFrom(address, length, sendStop ? I2C_STOP : I2C_NOSTOP,
			     1000000);
	if (!n)
		return 0;

	for (i = 0; i < n; i++)
		data[i] = Wire.readByte();

	return i;
}

uint8_t twi_writeTo(uint8_t address, uint8_t *data, uint8_t length,
		    uint8_t wait, uint8_t sendStop)
{
	unsigned int i = 0;

	Wire.beginTransmission(address);
	for (i = 0; i < length; i++)
		Wire.write(data[i]);
	Wire.endTransmission(sendStop ? I2C_STOP : I2C_NOSTOP, 1000000);

	return 0;
}

void twi_stop(void)
{
}
