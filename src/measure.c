//
// Power Measurement
//
// © Copyright 2019-2020, 2022 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include "board.h"
#include "cmd.h"
#include "ina219.h"
#include "measure.h"
#include "print.h"
#include "task.h"
#include "util.h"

#define FSHIFT		11		/* nr of bits of precision */
#define FIXED_1		(1 << FSHIFT)	/* 1.0 as fixed-point */
#define EXP_1		1884		/* 1/exp(5sec/1min) as fixed-point */
#define EXP_5		2014		/* 1/exp(5sec/5min) */
#define EXP_15		2037		/* 1/exp(5sec/15min) */

#define CALC_LOAD(load,exp,n) \
	load *= exp; \
	load += n * (FIXED_1 - exp); \
	load >>= FSHIFT;

struct avgs {
	unsigned int curr;
	unsigned int avg1;
	unsigned int avg5;
	unsigned int avg15;
};

static struct avgs vbus[NUM_POWER_CH] = { { 0xffffffff, }, { 0xffffffff, } };
static struct avgs vshunt[NUM_POWER_CH] = { { 0xffffffff, }, { 0xffffffff, } };
static struct avgs power[NUM_POWER_CH] = { { 0xffffffff, }, { 0xffffffff, } };
static struct avgs current[NUM_POWER_CH] = { { 0xffffffff, }, { 0xffffffff, } };

static void avgs_update(struct avgs *avgs, unsigned int val)
{
	if (avgs->curr == 0xffffffff) {
		avgs->avg1 = avgs->avg5 = avgs->avg15 = avgs->curr = val;
		return;
	}

	avgs->curr = val;
	CALC_LOAD(avgs->avg1, EXP_1, val);
	CALC_LOAD(avgs->avg5, EXP_5, val);
	CALC_LOAD(avgs->avg15, EXP_15, val);
}

static unsigned int ina219_probed;

static int measure(void)
{
	unsigned int ch, ch2;
	static int n;

	for (ch = 0; ch < NUM_POWER_CH; ch++) {
		if (!(ina219_probed & BIT(ch)))
			continue;

		ch2 = ina219_map[ch];
		avgs_update(&vbus[ch], ina219_get_bus_mV(ch2));
		avgs_update(&vshunt[ch], ina219_get_shunt_uV(ch2));
		avgs_update(&power[ch], ina219_get_power_mW(ch2));
		avgs_update(&current[ch], ina219_get_current_mA(ch2));
	}

	if (cmd_mode != CMD_MONITOR)
		return 0;

	if (!(n++ % 20))
		printf("     Vbus      Vshunt     Power                         Current\n"
		       "   --------  ---------  ----------------------------  ------------------------\n");

	for (ch = 0; ch < NUM_POWER_CH; ch++) {
		if (!(ina219_probed & BIT(ch)))
			continue;

		printf("%c: %5u mV  %3u.%02u mV  %5u mW (%5u %5u %5u)  %4u mA (%4u %4u %4u)\n",
		       'A' + ch,
		       vbus[ch].curr,
		       vshunt[ch].curr / 1000, vshunt[ch].curr % 1000 / 10,
		       power[ch].curr,
		       power[ch].avg1, power[ch].avg5, power[ch].avg15,
		       current[ch].curr,
		       current[ch].avg1, current[ch].avg5, current[ch].avg15);
	}

	return 0;
}

void measure_channel(unsigned int ch, unsigned int *mV, unsigned int *mA,
		     unsigned int *mW)
{
	if (ch > NUM_POWER_CH || !(ina219_probed & BIT(ch))) {
		*mV = *mA = *mW = 0;
		return;
	}

	*mV = vbus[ch].curr;
	*mA = current[ch].curr;
	*mW = power[ch].curr;
}

static struct task task_measure = {
	.name = "measure",
	.func = measure,
	.period = HZ,
};

void measure_init(void)
{
	unsigned int ch;
	int x;

	for (ch = 0; ch < NUM_POWER_CH; ch++) {
		x = ina219_init(ina219_map[ch]);
		if (x < 0) {
			pr_err("Initialization of INA219-%u failed: %d\n", ch,
			       x);
			continue;
		}

		ina219_probed |= BIT(ch);
	}

	if (ina219_probed)
		task_add(&task_measure);
}
