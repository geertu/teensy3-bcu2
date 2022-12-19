//
// Power Measurement
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

extern void measure_init(void);
extern void measure_channel(unsigned int ch, unsigned int *mV,
			    unsigned int *mA, unsigned int *mW);
