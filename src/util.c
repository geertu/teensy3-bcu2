//
// Utilities
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <string.h>

#include "util.h"

/* Partial strncasecmp() (s1 may be an abbreviation for s2) */
int part_strncasecmp(const char *s1, const char *s2, size_t min_match)
{
	size_t n1 = strlen(s1);
	size_t n2 = strlen(s2);

	return (n1 <= n2 && n1 >= min_match) ? strncasecmp(s1, s2, n1) : 1;
}

