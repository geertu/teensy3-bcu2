//
// RGB LED and Color Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <WProgram.h>

#include "board.h"
#include "rgb.h"

static void analogWriteInv(unsigned int pin, unsigned int val)
{
	val ^= 0xff;

	// val range is [0..255], but PWM range is [0..256]
	val += val >> 7;
	analogWrite(pin, val);
}

void rgb_write(unsigned int ch, unsigned int rgb)
{
	unsigned int base = ch * 3;

	analogWriteInv(pin_rgb[base + 2], rgb & 0xff);
	rgb >>= 8;
	analogWriteInv(pin_rgb[base + 1], rgb & 0xff);
	rgb >>= 8;
	analogWriteInv(pin_rgb[base + 0], rgb & 0xff);
}

const struct rgb_color rgb_colors[] = {
	{ "White",			0xffffff },
	{ "Red",			0xff0000 },
	{ "Orange",			0xff7f00 },
	{ "Yellow",			0xffff00 },
	{ "Chartreuse",			0x7fff00 },
	{ "Green",			0x00ff00 },
	{ "Spring green",		0x00ff7f },
	{ "CYan",			0x00ffff },
	{ "Azure",			0x007fff },
	{ "Blue",			0x0000ff },
	{ "Violet",			0x7f00ff },
	{ "Magenta",			0xff00ff },
	{ "ROse",			0xff007f },
	{ "BLAck",			0x000000 },
	{ NULL, }
};


/*****************************************************************************/

