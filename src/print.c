//
// Print Helpers
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <stdarg.h>
#include <stdio.h>

#include "print.h"

#define PRINTF_BUF_SIZE	256

#define DEF_PUTS(prefix)				\
int prefix ## _puts(const char *s)			\
{							\
	int i;						\
							\
	for (i = 0; s[i]; i++) {			\
		if (s[i] == '\r')			\
			prefix ## _putchar('\n');	\
		prefix ## _putchar(s[i]);		\
		if (s[i] == '\n')			\
			prefix ## _putchar('\r');	\
	}						\
	return i;					\
}

DEF_PUTS(serial);
DEF_PUTS(serial2);
DEF_PUTS(usb_serial);

#define DEF_PRINTF(prefix)				\
int prefix ## _printf(const char *fmt, ...)		\
{							\
	static char buf[PRINTF_BUF_SIZE];		\
	va_list args;					\
	int n;						\
							\
	va_start(args, fmt);				\
	n = vsnprintf(buf, sizeof(buf), fmt, args);	\
	va_end(args);					\
	if (n < 0)					\
		return n;				\
							\
	return prefix ## _puts(buf);			\
}

DEF_PRINTF(serial);
DEF_PRINTF(serial2);
DEF_PRINTF(usb_serial);
