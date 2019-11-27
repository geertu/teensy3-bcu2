//
// Environment Handling
//
// © Copyright 2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include <string.h>

#include "env.h"
#include "print.h"

static struct env {
	const char *key;
	const char *val;
} environment[] = {
	{ "prompt", "BFF> " },
	{ "baudA", "115200" },
	{ "baudB", "115200" },
	/* sentinel */
	{ NULL, NULL },
};

void env_init(void)
{
	// TODO read environment from non-volatile memory
}

const char *env_get(const char *key)
{
	const struct env *env;

	for (env = environment; env->key; env++) {
		if (!strcmp(env->key, key))
			return env->val;
	}

	return NULL;
}

void env_set(const char *key, const char *val)
{
	struct env *env;

	for (env = environment; env->key; env++) {
		if (!strcmp(env->key, key)) {
			env->val = strdup(val);
			return;
		}
	}

	pr_warn("%s: Not yet implemented\n", __func__);
}

void env_print(void)
{
	const struct env *env;

	for (env = environment; env->key; env++)
		printf("%s = %s\n", env->key, env->val);
}

void env_save(void)
{
	// TODO save environment to non-volatile memory
	pr_warn("%s: Not yet implemented\n", __func__);
}
