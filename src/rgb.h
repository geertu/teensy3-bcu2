//
// RGB LED and Color Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

extern void rgb_write(unsigned int ch, unsigned int rgb);

struct rgb_color {
	const char *name;
	unsigned int rgb;
};

extern const struct rgb_color rgb_colors[];
