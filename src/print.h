//
// Print Helpers
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <WProgram.h>
#include <usb_serial.h>

#define ESC_NORMAL		"\e[0m"
#define ESC_BLACK		"\e[31m"
#define ESC_RED			"\e[31m"
#define ESC_GREEN		"\e[32m"
#define ESC_YELLOW		"\e[33m"
#define ESC_BLUE		"\e[34m"
#define ESC_MAGENTA		"\e[35m"
#define ESC_CYAN		"\e[36m"
#define ESC_WHITE		"\e[37m"

#define __printf(a, b)	__attribute__((__format__(printf, a, b)))

extern int serial_puts(const char *s);
extern int serial2_puts(const char *s);

extern int usb_serial_puts(const char *s);

extern __printf(1, 2) int serial_printf(const char *fmt, ...);
extern __printf(1, 2) int serial2_printf(const char *fmt, ...);

extern __printf(1, 2) int usb_serial_printf(const char *fmt, ...);

static inline __printf(1, 2) int dummy_printf(const char *fmt, ...)
{
	return 0;
}

#undef putchar
#undef getchar
#define printf			usb_serial_printf
#define putchar			usb_serial_putchar
#define getchar			usb_serial_getchar

#define pr_info(fmt, ...)	printf(ESC_GREEN fmt ESC_NORMAL, ##__VA_ARGS__)
#define pr_warn(fmt, ...)	printf(ESC_YELLOW fmt ESC_NORMAL, ##__VA_ARGS__)
#define pr_err(fmt, ...)	printf(ESC_RED fmt ESC_NORMAL, ##__VA_ARGS__)

#ifdef DEBUG
#define pr_debug		printf(ESC_BLUE fmt ESC_NORMAL, ##__VA_ARGS__)
#else
#define pr_debug		dummy_printf
#endif
