//
// Command Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <ctype.h>
#include <stdlib.h>
#include <twi.h>
#include <usb_names.h>

#include "board.h"
#include "cmd.h"
#include "env.h"
#include "input.h"
#include "print.h"
#include "rgb.h"
#include "util.h"
#include "version.h"

#define ARGV_MAX	10

#define I2C_ADDR_FIRST	0x03	/* First address to scan on the I2C bus */
#define I2C_ADDR_LAST	0x77	/* Last address to scan on the I2C bus */

int cmd_mode = CMD_COMMAND;

struct cmd {
	const char *name;
	const char *help;
	void (*func)(int argc, char *argv[]);
};

static struct cmd commands[];

static int decode_channel(const char *arg, const char *name, unsigned int n)
{
	char c;

	if (!part_strncasecmp(arg, "ALL", 2))
		return -1;

	if (arg[1])
		goto error;

	c = arg[0];
	if (c >= '0' && c < '0' + n)
		return c - '0';

	if (c >= 'A' && c < 'A' + n)
		return c - 'A';

	if (c >= 'a' && c < 'a' + n)
		return c - 'a';

error:
	printf("Invalid %s channel %s\n", name, arg);
	return -2;
}

enum state {
	STATE_OFF = 0,
	STATE_ON,
	STATE_PULSE
};

static int decode_state(const char *arg, const char *name, int allow_pulse)
{
	if (!part_strncasecmp(arg, "ON", 2) || !strcmp(arg, "1"))
		return STATE_ON;

	if (!part_strncasecmp(arg, "OFF", 2) || !strcmp(arg, "0"))
		return STATE_OFF;

	if (allow_pulse && !part_strncasecmp(arg, "PULSE", 1))
		return STATE_PULSE;

	printf("Invalid %s state %s\n", name, arg);
	return -1;
}

static void cmd_help(int argc, char *argv[])
{
	const struct cmd *cmd;

	printf("Welcome to BFF, the Board Farm Frobnicator!\n\n");
	printf("Valid command are:\n");
	for (cmd = commands; cmd->name; cmd++)
		printf("    %s: %s\n", cmd->name, cmd->help);
}

static void cmd_history(int argc, char *argv[])
{
	input_show_history();
}

extern struct usb_string_descriptor_struct usb_string_serial_number;

static void cmd_version(int argc, char *argv[])
{
	char sn[usb_string_serial_number.bLength / 2];
	unsigned int i;

	/* usb_string_serial_number_default is an ASCII number */
	for (i = 0; i < sizeof(sn); i++)
		sn[i] = usb_string_serial_number.wString[i];

	printf("BFF version %s, Device SerialNumber %s\n", gitversion, sn);
}

static void cmd_getenv(int argc, char *argv[])
{
	const char *key, *val;

	if (argc != 1) {
		printf("Usage: getenv <key>\n");
		return;
	}

	key = argv[0];
	val = env_get(key);
	if (val)
		printf("%s = %s\n", key, val);
	else
		printf("%s is not defined\n", key);
}

static void cmd_setenv(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: setenv <key> <val>\n");
		return;
	}

	env_set(argv[0], argv[1]);
}

static void cmd_printenv(int argc, char *argv[])
{
	env_print();
}

static void cmd_saveenv(int argc, char *argv[])
{
	env_save();
}

static void cmd_start(int mode, const char *name)
{
	printf("Starting %s (CTRL-C to interrupt)\n", name);
	cmd_mode = mode;
}

static void cmd_monitor(int argc, char *argv[])
{
	cmd_start(CMD_MONITOR, "power monitor");
}

static void cmd_test(int argc, char *argv[])
{
	cmd_start(CMD_TEST, "test");
}

static int decode_hex_char(char c)
{
	switch (c) {
	case '0'...'9':
		return c - '0';

	case 'a'...'f':
		return 10 + c - 'a';

	case 'A'...'F':
		return 10 + c - 'A';

	default:
		return -1;
	}
}

#define for_each_selected_channel(_i, _ch, _num_ch) \
	for (_i = (_ch < 0 ? 0 : _ch); _i < (_ch < 0 ? _num_ch : _ch + 1); _i++)

static void cmd_rgb(int argc, char *argv[])
{
	static unsigned int cache[NUM_RGB_CH];
	unsigned int rgb = 0, i;
	const char *color;
	size_t n;
	int ch, x;

	if (argc == 1 && !part_strncasecmp(argv[0], "list", 1)) {
		for (i = 0; rgb_colors[i].name; i++)
			printf("#%06x %s\n", rgb_colors[i].rgb,
			       rgb_colors[i].name);
		return;
	}

	if (argc < 1 || argc > 2 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: rgb <list> | <channel> [<colorname> | #rgb | #rrggbb]\n\n");
		printf("Valid channels are A..%c|0..%u|ALL\n",
		       'A' + NUM_RGB_CH - 1, NUM_RGB_CH - 1);
		return;
	}

	ch = decode_channel(argv[0], "rgb", NUM_RGB_CH);
	if (ch < -1)
		return;

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_RGB_CH)
			printf("#%06x\n", cache[i]);
		return;
	}

	color = argv[1];
	if (color[0] != '#') {
		for (i = 0; rgb_colors[i].name; i++)
			if (!part_strncasecmp(color, rgb_colors[i].name, 1)) {
				color = rgb_colors[i].name;
				rgb = rgb_colors[i].rgb;
				goto found;
			}
		printf("Unknown color %s\n", color);
		return;
	}

	n = strlen(color);
	if (n != 4 && n != 7) {
		printf("Invalid RGB color %s\n", color);
		return;
	}

	for (i = 1; i < n; i++) {
		x = decode_hex_char(color[i]);
		if (x < 0) {
			printf("Invalid hex number %s\n", color);
			return;
		}
		rgb = (rgb << 4) | x;
		if (n == 4)
			rgb = (rgb << 4) | x;
	}

found:
	for_each_selected_channel(i, ch, NUM_RGB_CH) {
		printf("Showing color %s on channel %c\n", color, 'A' + i);
		rgb_write(i, rgb);
		cache[i] = rgb;
	}
	return;
}

static void cmd_power(int argc, char *argv[])
{
	static char cache[NUM_POWER_CH];
	unsigned int i;
	int ch, state;

	if (argc < 1 || argc > 2 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: power <channel> [<state>]\n\n");
		printf("Valid channels are A..%c|0..%u|ALL\n",
		       'A' + NUM_POWER_CH - 1, NUM_POWER_CH - 1);
		printf("Valid states are ON|OFF|1|0\n");
		return;
	}

	ch = decode_channel(argv[0], "power", NUM_POWER_CH);
	if (ch < -1)
		return;

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_POWER_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], "power", false);
	if (state < 0)
		return;

	for_each_selected_channel(i, ch, NUM_POWER_CH) {
		printf("Powering channel %c %s\n", 'A' + i,
		       state ? "on" : "off");
		digitalWrite(pin_power[i], state);
		cache[i] = state;
	}
}

#define KEY_PULSE_MS	200

static void cmd_key(int argc, char *argv[])
{
	static char cache[NUM_KEY_CH];
	unsigned int i;
	int ch, state;

	if (argc < 1 || argc > 2 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: key <channel> [<state>]\n\n");
		printf("Valid channels are A..%c|0..%u|ALL\n",
		       'A' + NUM_KEY_CH - 1, NUM_KEY_CH - 1);
		printf("Valid states are ON|OFF|PULSE|1|0\n");
		return;
	}

	ch = decode_channel(argv[0], "key", NUM_KEY_CH);
	if (ch < -1)
		return;

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_KEY_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], "key", true);
	if (state < 0)
		return;

	for_each_selected_channel(i, ch, NUM_KEY_CH) {
		/* Keys are active-low! */
		switch (state) {
		case STATE_ON:
			printf("Switching key %c %s\n", '0' + i, "on");
			digitalWrite(pin_key[i], 0);
			cache[i] = 1;
			break;

		case STATE_OFF:
			printf("Switching key %c %s\n", '0' + i, "off");
			digitalWrite(pin_key[i], 1);
			cache[i] = 0;
			break;

		case STATE_PULSE:
			printf("Pulsing key %c\n", '0' + i);
			digitalWrite(pin_key[i], 0);
			delay(KEY_PULSE_MS);
			digitalWrite(pin_key[i], 1);
			cache[i] = 0;
			break;
		}
	}
}

static void cmd_gpio(int argc, char *argv[])
{
	static char cache[NUM_GPIO_CH];
	unsigned int i;
	int ch, state;

	if (argc < 1 || argc > 2 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: gpio <channel> <state>\n\n");
		printf("Valid channels are A..%c|0..%u|ALL\n",
		       'A' + NUM_GPIO_CH - 1, NUM_GPIO_CH - 1);
		printf("Valid states are ON|OFF|PULSE|1|0\n");
		return;
	}

	ch = decode_channel(argv[0], "gpio", NUM_GPIO_CH);
	if (ch < -1)
		return;

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_GPIO_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], "gpio", true);
	if (state < 0)
		return;

	for_each_selected_channel(i, ch, NUM_GPIO_CH) {
		switch (state) {
		case STATE_ON:
			printf("Switching GPIO %c %s\n", '0' + i, "on");
			digitalWrite(pin_gpio[i], 1);
			cache[i] = 1;
			break;

		case STATE_OFF:
			printf("Switching GPIO %c %s\n", '0' + i, "off");
			digitalWrite(pin_gpio[i], 0);
			cache[i] = 0;
			break;

		case STATE_PULSE:
			printf("Pulsing GPIO %c\n", '0' + i);
			digitalWrite(pin_gpio[i], 1);
			delay(KEY_PULSE_MS);
			digitalWrite(pin_gpio[i], 0);
			cache[i] = 0;
			break;
		}
	}
}

static void cmd_i2c_scan(void)
{
	unsigned int i, n;
	int res;

	for (i = I2C_ADDR_FIRST, n = 0; i <= I2C_ADDR_LAST; i++) {
		res = twi_writeTo(i, NULL, 0, true, true);
		switch (res) {
		case 0:
			printf("Found I2C device at address %#02x\n", i);
			n++;
			break;

		case 2:	/* recv addr NACK */
			break;

		default:
			pr_err("I2C bus failure\n");
			return;
		}
	}

	if (!n)
		printf("No I2C devices found\n");
}

static void cmd_i2c_get(int argc, char *argv[])
{
	unsigned int addr, n = 1;
	uint8_t reg, buf[4];
	int res;

	if (argc < 1 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: i2c get <addr> [<reg> [b|w|l]]\n");
		return;
	}

	addr = strtoul(argv[0], NULL, 0);

	if (argc > 1)
		reg = strtoul(argv[1], NULL, 0);

	if (argc > 2) {
		switch (argv[2][0]) {
		case 'b':
			n = 1;
			break;
		case 'w':
			n = 2;
			break;
		case 'l':
			n = 4;
			break;
		default:
			printf("Invalid mode %s\n", argv[2]);
			return;
		}
		if (argv[2][1]) {
			printf("Invalid mode %s\n", argv[2]);
			return;
		}
	}

	if (argc > 1) {
		res = twi_writeTo(addr, &reg, 1, true, false);
		if (res) {
			pr_err("I2C write failed %d\n", res);
			return;
		}
	}

	res = twi_readFrom(addr, buf, n, true);
	if (res != n) {
		pr_err("I2C read returned only %d bytes\n", res);
		return;
	}

	switch (n) {
	case 1:
		printf("%#02x\n", buf[0]);
		break;
	case 2:
		printf("%#04x\n", buf[0] << 8 |  buf[1]);
		break;
	case 4:
		printf("%#08x\n",
		       buf[0] << 24 |  buf[1] << 16 | buf[2] << 8 | buf[3]);
		break;
	}
}

static void cmd_i2c_set(int argc, char *argv[])
{
	uint8_t data[ARGV_MAX - 3];
	unsigned int addr, i;
	int res;

	if (argc < 1 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: i2c set <addr> [<reg> [<data> ...]]\n");
		return;
	}

	addr = strtoul(argv[0], NULL, 0);

	for (i = 0; i < argc - 1; i++)
		data[i] = strtoul(argv[i + 1], NULL, 0);

	res = twi_writeTo(addr, data, argc - 1, true, true);
	if (res)
		pr_err("I2C write failed %d\n", res);
}

static void cmd_i2c(int argc, char *argv[])
{
	if (argc < 1 || !part_strncasecmp(argv[0], "help", 1)) {
		printf("Usage: i2c <cmd> ...\n\n");
		printf("Valid commands are: Scan, Get, SEt\n");
		return;
	}

	if (!part_strncasecmp(argv[0], "scan", 1))
		cmd_i2c_scan();
	else if (!part_strncasecmp(argv[0], "get", 1))
		cmd_i2c_get(argc - 1, argv + 1);
	else if (!part_strncasecmp(argv[0], "set", 1))
		cmd_i2c_set(argc - 1, argv + 1);
	else
		printf("Unknown I2C command %s\n", argv[0]);
}

static struct cmd commands[] = {
	{ "Getenv", "Get the value of an environment variable", cmd_getenv },
	{ "GPio", "Control GPIO", cmd_gpio },
	{ "Help", "Display this help", cmd_help },
	{ "HIstory", "Show command history", cmd_history },
	{ "I2c", "I2C tools", cmd_i2c },
	{ "Key", "Control key", cmd_key },
	{ "Monitor", "Monitor power consumption", cmd_monitor },
	{ "Power", "Control power", cmd_power },
	{ "PRintenv", "Print all environment variables", cmd_printenv },
	{ "RGB", "Show a color", cmd_rgb },
	{ "Saveenv", "Save all environment variables", cmd_saveenv },
	{ "SEtenv", "Set the value of an environment variable", cmd_setenv },
	{ "Test", "Test cycle through board features", cmd_test },
	{ "Version", "Display software version", cmd_version },
	{ NULL, },
};

void cmd_prompt(void)
{
	printf("%s", env_get("prompt") ?: "");
}

void cmd_run(char *line)
{
	static char *argv[ARGV_MAX];
	unsigned int argc;
	const struct cmd *cmd;
	char quote;

	for (argc = 0; argc < ARRAY_SIZE(argv); argc++) {
		quote = '\0';
		while (isspace(*line))
			line++;
		if (!*line)
			break;
		if (*line == '\'' || *line == '"')
			quote = *line++;
		argv[argc] = line;
		if (quote) {
			while (*line && *line != quote)
				line++;
		} else {
			while (*line && !isspace(*line))
				line++;
		}
		if (*line)
			*line++ = '\0';
	}
	if (!argc)
		goto prompt;

	if (*line) {
		printf("Too many arguments (max = %u)\n", ARGV_MAX);
		goto prompt;
	}

	if (argv[0][0] == '?') {
		cmd_help(argc - 1, argv + 1);
		goto prompt;
	}

	for (cmd = commands; cmd->name; cmd++)
		if (!part_strncasecmp(argv[0], cmd->name, 1)) {
			cmd->func(argc - 1, argv + 1);
			goto prompt;
		}

	printf("Unknown command\n");

prompt:
	if (cmd_mode == CMD_COMMAND)
		cmd_prompt();
}
