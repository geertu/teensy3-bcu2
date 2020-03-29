//
// Main Board Control Unit Duo Program
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <twi.h>
#include <usb_names.h>
#include <usb_serial2.h>
#include <usb_serial3.h>

#include "board.h"
#include "cmd.h"
#include "env.h"
#include "event.h"
#include "measure.h"
#include "print.h"
#include "rgb.h"
#include "task.h"
#include "util.h"

/*****************************************************************************/

#define DEFAULT_BAUD		115200

#define MAX_SERIAL_BURST	64

/*****************************************************************************/

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
        2 + 9 * 2,
        3,
        { 'G', 'l', 'i', 'd', 'e', 'r', ' ', 'b', 'v' }
};
struct usb_string_descriptor_struct usb_string_product_name = {
	2 + 5 * 2,
        3,
        { 'B', 'C', 'U', '/', '2' }
};

/*****************************************************************************/

void serial_event(void)
{
	unsigned int i;
	int c;

	for (i = 0; i < MAX_SERIAL_BURST; i++) {
		c = serial_getchar();
		if (c < 0)
			return;

		usb_serial2_putchar(c);
	}
}

void serial2_event(void)
{
	unsigned int i;
	int c;

	for (i = 0; i < MAX_SERIAL_BURST; i++) {
		c = serial2_getchar();
		if (c < 0)
			return;

		usb_serial3_putchar(c);
	}
}

static unsigned int get_baud(const char *key)
{
	const char *var = env_get(key) ?: env_get("baud");
	unsigned int baud = var ? atoi(var) : DEFAULT_BAUD;

	return baud ?: DEFAULT_BAUD;
}

static void console_init(void)
{
	serial_begin(BAUD2DIV(get_baud("baudA")));
	serial2_begin(BAUD2DIV2(get_baud("baudB")));
}

/*****************************************************************************/

#define LINE_MAX	80

static void bell(void)
{
	putchar('\a');
}

void usb_serial_event(void)
{
	static char input_buf[LINE_MAX + 1];
	static unsigned int input_len;
	unsigned int i;
	int c;

	for (i = 0; i < MAX_SERIAL_BURST; i++) {
		c = getchar();
		if (c < 0)
			return;

		if (c == '\003') {
			cmd_prompt();
			cmd_mode = CMD_COMMAND;
			continue;
		}

		if (cmd_mode != CMD_COMMAND)
			continue;

		switch (c) {
		case '\b':
		case 0x7f:
			/* Backspace */
			if (!input_len) {
				bell();
				break;
			}

			printf("\b \b");
			input_len--;
			break;

		case '\n':
		case '\r':
			/* Enter */
			printf("\n");
			input_buf[input_len] = 0;
			cmd_run(input_buf);
			input_len = 0;
			break;

		case ' '...'~':
			/* Vanilla characters */
			if (input_len >= sizeof(input_buf) - 1) {
				bell();
				break;
			}

			putchar(c);
			input_buf[input_len++] = c;
			break;

		default:
			pr_warn("Unhandled special character 0x%x\n", c);
			break;
		}
	}
}

void usb_serial2_event(void)
{
	unsigned int i;
	int c;

	for (i = 0; i < MAX_SERIAL_BURST; i++) {
		c = usb_serial2_getchar();
		if (c < 0)
			return;

		serial_putchar(c);
	}
}

void usb_serial3_event(void)
{
	unsigned int i;
	int c;

	for (i = 0; i < MAX_SERIAL_BURST; i++) {
		c = usb_serial3_getchar();
		if (c < 0)
			return;

		serial2_putchar(c);
	}
}

static void input_init(void)
{
	cmd_prompt();
}

/*****************************************************************************/

static void test(void)
{
	static const char *comp[3] = { "red", "green", "blue" };
	static unsigned int state, cnt;
	unsigned int ch, i;

	ch = state++;
	if (ch < NUM_RGB_CH * 3) {
		i = ch % 3;
		ch /= 3;
		printf("RGB channel %c: %s\n", 'A' + ch, comp[i]);
		if (ch)
			rgb_write(ch - 1, 0);
		rgb_write(ch, 0xff << (2 - i) * 8);
		return;
	}

	ch -= NUM_RGB_CH * 3;
	if (ch < NUM_KEY_CH) {
		if (!ch)
			rgb_write(NUM_RGB_CH - 1, 0);
		else
			digitalWrite(pin_key[ch - 1], 1);

		printf("Pulsing key %c\n", '0' + ch);
		digitalWrite(pin_key[ch], 0);
		return;
	}

	ch -= NUM_KEY_CH;
	if (ch < NUM_POWER_CH) {
		if (!ch)
			digitalWrite(pin_key[NUM_KEY_CH - 1], 1);
		else
			digitalWrite(pin_power[ch - 1], 0);

		printf("Powering channel %c\n", 'A' + ch);
		digitalWrite(pin_power[ch], 1);
		return;
	}

	ch -= NUM_POWER_CH;
	if (ch < NUM_GPIO_CH) {
		if (!ch)
			digitalWrite(pin_power[NUM_POWER_CH - 1], 0);
		else
			digitalWrite(pin_gpio[ch - 1], 0);

		printf("Pulsing GPIO %c\n", '0' + ch);
		digitalWrite(pin_gpio[ch], 1);
		return;
	}

	ch -= NUM_GPIO_CH;
	if (ch < NUM_UART_CH) {
		if (!ch)
			digitalWrite(pin_gpio[NUM_GPIO_CH - 1], 0);

		printf("Saying hello to UART channel %c\n", 'A' + ch);
		if (!ch)
			serial_printf("Hello %c %u\n", 'A' + ch, cnt++);
		else
			serial2_printf("Hello %c %u\n", 'A' + ch, cnt++);
		return;
	}

	state = 0;
}

static int blink(void)
{
	static unsigned int state;

	switch (state % 10) {
	case 0:
		if (cmd_mode == CMD_TEST)
			test();
		break;

	case 1:
	case 4:
		digitalWrite(pin_heartbeat, 1);
		break;

	case 2:
	case 6:
		digitalWrite(pin_heartbeat, 0);
		break;
	}

	state++;
	return 0;
}

static struct task task_heartbeat = {
	.name = "heartbeat",
	.func = blink,
	.period = HZ / 10,
};

static void leds_init(void)
{
	unsigned int i;

	pinMode(pin_heartbeat, OUTPUT);

	for (i = 0; i < NUM_POWER_CH; i++) {
		pinMode(pin_power[i], OUTPUT);
		digitalWrite(pin_power[i], 0);
	}

	for (i = 0; i < NUM_KEY_CH; i++) {
		pinMode(pin_key[i], OUTPUT);
		digitalWrite(pin_key[i], 1);
	}

	for (i = 0; i < NUM_GPIO_CH; i++) {
		pinMode(pin_gpio[i], OUTPUT);
		digitalWrite(pin_gpio[i], 0);
	}

	for (i = 0; i < NUM_RGB_CH * 3; i++) {
		pinMode(pin_rgb[i], OUTPUT);
		digitalWrite(pin_rgb[i], 1);
	}

	task_add(&task_heartbeat);
}

/*****************************************************************************/

int main(int argc, char *argv[])
{
	env_init();
	leds_init();
	twi_init();
	measure_init();
	console_init();
	input_init();

	task_run_loop();

	twi_stop();
}
