//
// Utilities
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <stddef.h>
#include <stdint.h>

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define BIT(x)			(1U << (x))

#define DIV_ROUND_CLOSEST(x, divisor)(                  \
{                                                       \
	typeof(x) __x = x;                              \
	typeof(divisor) __d = divisor;                  \
	(((typeof(x))-1) > 0 ||                         \
	 ((typeof(divisor))-1) > 0 ||                   \
	 (((__x) > 0) == ((__d) > 0))) ?                \
		(((__x) + ((__d) / 2)) / (__d)) :       \
		(((__x) - ((__d) / 2)) / (__d));        \
}                                                       \
)

extern int part_strncasecmp(const char *s1, const char *s2, size_t min_match);
