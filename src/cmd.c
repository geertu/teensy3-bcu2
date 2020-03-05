//
// Command Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <ctype.h>
#include <usb_names.h>

#include "board.h"
#include "cmd.h"
#include "env.h"
#include "print.h"
#include "rgb.h"
#include "util.h"
#include "version.h"

#define ARGV_MAX	10

int cmd_mode = CMD_COMMAND;

struct cmd {
	const char *name;
	const char *help;
	void (*func)(int argc, char *argv[]);
};

static struct cmd commands[];

static int decode_channel(const char *s, unsigned int n)
{
	char c;

	if (!part_strncasecmp(s, "ALL", 2))
		return -1;

	if (s[1])
		return -2;

	c = s[0];
	if (c >= '0' && c < '0' + n)
		return c - '0';

	if (c >= 'A' && c < 'A' + n)
		return c - 'A';

	if (c >= 'a' && c < 'a' + n)
		return c - 'a';

	return -2;
}

enum state {
	STATE_OFF = 0,
	STATE_ON,
	STATE_PULSE
};

static int decode_state(const char *s, int allow_pulse)
{
	if (!part_strncasecmp(s, "ON", 2) || !strcmp(s, "1"))
		return STATE_ON;

	if (!part_strncasecmp(s, "OFF", 2) || !strcmp(s, "0"))
		return STATE_OFF;

	if (allow_pulse && !part_strncasecmp(s, "PULSE", 1))
		return STATE_PULSE;

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

	ch = decode_channel(argv[0], NUM_RGB_CH);
	if (ch < -1) {
		printf("Invalid RGB channel %s\n", argv[0]);
		return;
	}

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
	printf("Showing color %s\n", color);
	for_each_selected_channel(i, ch, NUM_RGB_CH) {
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

	ch = decode_channel(argv[0], NUM_POWER_CH);
	if (ch < -1) {
		printf("Invalid power channel %s\n", argv[0]);
		return;
	}

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_POWER_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], false);
	if (state < 0) {
		printf("Invalid power state %s\n", argv[1]);
		return;
	}

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
		printf("Usage: keys <channel> [<state>]\n\n");
		printf("Valid channels are A..%c|0..%u|ALL\n",
		       'A' + NUM_KEY_CH - 1, NUM_KEY_CH - 1);
		printf("Valid states are ON|OFF|PULSE|1|0\n");
		return;
	}

	ch = decode_channel(argv[0], NUM_KEY_CH);
	if (ch < -1) {
		printf("Invalid key %s\n", argv[0]);
		return;
	}

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_KEY_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], true);
	if (state < 0) {
		printf("Invalid key state %s\n", argv[1]);
		return;
	}

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

	ch = decode_channel(argv[0], NUM_GPIO_CH);
	if (ch < -1) {
		printf("Invalid gpio %s\n", argv[0]);
		return;
	}

	if (argc < 2) {
		for_each_selected_channel(i, ch, NUM_GPIO_CH)
			printf("%d\n", cache[i]);
		return;
	}

	state = decode_state(argv[1], true);
	if (state < 0) {
		printf("Invalid gpio state %s\n", argv[1]);
		return;
	}

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

static struct cmd commands[] = {
	{ "Getenv", "Get the value of an environment variable", cmd_getenv },
	{ "GPio", "Control GPIO", cmd_gpio },
	{ "Help", "Display this help", cmd_help },
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
