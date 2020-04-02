//
// Input Handling
//
// © Copyright 2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <ctype.h>

#include "cmd.h"
#include "input.h"
#include "print.h"

#define LINE_MAX	80

enum input_state {
	INPUT_NORMAL,
	INPUT_ESC,
	INPUT_CSI,
	INPUT_TERM,
};

static enum input_state state = INPUT_NORMAL;
static char input_buf[LINE_MAX + 1];
static unsigned int input_len;	/* Number of characters in input buffer */
static unsigned int input_pos;	/* Current cursor position in input buffer */

static void bell(void)
{
	putchar('\a');
}

static void move_left(unsigned int n)
{
	while (n--)
		putchar('\b');
}

static void spaces(unsigned int n)
{
	while (n--)
		putchar(' ');
}

void input_handle(char c)
{
	unsigned int n;

	if (c == '\003') {
		/* CTRL-C */
		cmd_prompt();
		cmd_mode = CMD_COMMAND;
		return;
	}

	if (cmd_mode != CMD_COMMAND)
		return;

	switch (state) {
	case INPUT_NORMAL:
		switch (c) {
		case '\e':
			state = INPUT_ESC;
			break;

		case '\b':
		case 0x7f:
			/* Backspace */
			if (!input_pos) {
				bell();
				break;
			}

			n = 1;
			goto do_kill_word;

		case '\n':
		case '\r':
			/* Enter */
			printf("\n");
			cmd_run(input_buf);
			input_len = input_pos = 0;
			input_buf[0] = 0;
			break;

		case '\f':
			/* CTRL-L */
			printf("\ec");
			cmd_prompt();
			printf("%s", input_buf);
			move_left(input_len - input_pos);
			break;

		case 0x01:
			/* CTRL-A */
			goto do_home;

		case 0x04:
			/* CTRL-D */
			goto do_delete;

		case 0x05:
			/* CTRL-E */
			goto do_end;

		case 0x17:
			/* CTRL-W */
			if (!input_pos) {
				bell();
				break;
			}

			for (n = 1; n < input_pos; n++) {
				if (isspace(input_buf[input_pos - n - 1]))
					break;
			}

do_kill_word:
			memmove(&input_buf[input_pos - n],
				&input_buf[input_pos], input_len - input_pos);
			input_pos -= n;
			input_len -= n;
			input_buf[input_len] = 0;
			move_left(n);
			printf("%s", &input_buf[input_pos]);
			spaces(n);
			move_left(input_len - input_pos + n);
			break;

		case '\t':
			c = ' ';
			/* fall through */
		case ' '...'~':
			/* Vanilla characters */
			if (input_len >= sizeof(input_buf) - 1) {
				bell();
				break;
			}

			memmove(&input_buf[input_pos + 1],
				&input_buf[input_pos], input_len - input_pos);
			input_buf[input_pos] = c;
			input_pos++;
			input_len++;
			input_buf[input_len] = 0;
			printf("%s", &input_buf[input_pos - 1]);
			move_left(input_len - input_pos);
			break;

		default:
			pr_debug("Unhandled special character %#x\n", c);
			break;
		}
		break;

	case INPUT_ESC:
		switch (c) {
		case '[':
			state = INPUT_CSI;
			break;

		default:
			pr_debug("Unhandled ESC command %#x\n", c);
			state = INPUT_NORMAL;
			break;
		}
		break;

	case INPUT_CSI:
		switch (c) {
		case '1':
		case '7':
			/* Home */
			state = INPUT_TERM;
do_home:
			move_left(input_pos);
			input_pos = 0;
			break;

		case '2':
			/* Insert */
			state = INPUT_TERM;
			pr_debug("Insert!\n");
			break;

		case '3':
			/* Delete */
			state = INPUT_TERM;
do_delete:
			if (input_pos == input_len) {
				bell();
				break;
			}
			memmove(&input_buf[input_pos],
				&input_buf[input_pos + 1],
				input_len - input_pos - 1);
			input_len--;
			input_buf[input_len] = 0;
			printf("%s ", &input_buf[input_pos]);
			move_left(input_len - input_pos + 1);
			break;

		case '4':
		case '8':
			/* End */
			state = INPUT_TERM;
do_end:
			printf("%s", &input_buf[input_pos]);
			input_pos = input_len;
			break;

		case '5':
			/* PgUp */
			state = INPUT_TERM;
			pr_debug("PgUp!\n");
			break;

		case '6':
			/* PgDn */
			state = INPUT_TERM;
			pr_debug("PgDn!\n");
			break;

		case 'A':
			/* Up */
			state = INPUT_NORMAL;
			pr_debug("Up!\n");
			break;

		case 'B':
			/* Down */
			state = INPUT_NORMAL;
			pr_debug("Down!\n");
			break;

		case 'C':
			/* Right */
			state = INPUT_NORMAL;
			if (input_pos == input_len) {
				bell();
				break;
			}
			putchar(input_buf[input_pos++]);
			break;

		case 'D':
			/* Left */
			state = INPUT_NORMAL;
			if (!input_pos) {
				bell();
				break;
			}
			move_left(1);
			input_pos--;
			break;

		default:
			pr_debug("Unhandled CSI command %#x\n", c);
			state = INPUT_NORMAL;
			break;
		}
		break;

	case INPUT_TERM:
		switch (c) {
		case '~':
			/* Terminal input sequence */
			state = INPUT_NORMAL;
			break;

		default:
			pr_debug("Unhandled sequence terminator %#x\n", c);
			state = INPUT_NORMAL;
			break;
		}
		break;
	}
}
