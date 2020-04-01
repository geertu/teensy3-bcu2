//
// Input Handling
//
// © Copyright 2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include "cmd.h"
#include "input.h"
#include "print.h"

#define LINE_MAX	80

static char input_buf[LINE_MAX + 1];
static unsigned int input_len;

static void bell(void)
{
	putchar('\a');
}

void input_handle(char c)
{
	if (c == '\003') {
		cmd_prompt();
		cmd_mode = CMD_COMMAND;
		return;
	}

	if (cmd_mode != CMD_COMMAND)
		return;

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
		pr_warn("Unhandled special character %#x\n", c);
		break;
	}
}
