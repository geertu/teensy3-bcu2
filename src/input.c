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
#define HIST_MAX	20

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

static char hist_buf[HIST_MAX][LINE_MAX + 1];
static unsigned int hist_len;	/* Number of entries in history */
static unsigned int hist_idx;	/* Index to write next history entry */
static unsigned int hist_off;	/* Look-up read offset relative to index */

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

static int isspaces(const char *s)
{
	while (isspace(*s))
		s++;

	return !*s;
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
			if (!isspaces(input_buf)) {
				if (!hist_len ||
				    strcmp(hist_buf[hist_idx ? hist_idx - 1
							     : HIST_MAX - 1],
					   input_buf)) {
					strcpy(hist_buf[hist_idx], input_buf);
					hist_idx = (hist_idx + 1) % HIST_MAX;
					if (hist_len < HIST_MAX)
						hist_len++;
				}
				hist_off = 0;
			}
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
			if (hist_off == hist_len) {
				bell();
				break;
			}

			hist_off++;
			goto do_hist;

		case 'B':
			/* Down */
			state = INPUT_NORMAL;
			if (!hist_off) {
				bell();
				break;
			}

			hist_off--;
do_hist:
			if (!hist_off) {
				input_buf[0] = 0;
			} else {
				n = hist_idx - hist_off;
				if (hist_idx < hist_off)
					n += HIST_MAX;
				strcpy(input_buf, hist_buf[n]);
			}
			n = input_len;
			input_len = strlen(input_buf);
			move_left(input_pos);
			printf("%s", input_buf);
			if (n > input_len) {
				spaces(n - input_len);
				move_left(n - input_len);
			}
			input_pos = input_len;
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

void input_show_history(void)
{
	unsigned int i, j;

	j = hist_idx - hist_len;
	if (hist_idx < hist_len)
		j += HIST_MAX;
	for (i = 0; i < hist_len; i++) {
		printf("%3u %s\n", i + 1, hist_buf[j++]);
		if (j == HIST_MAX)
			j = 0;
	}
}
