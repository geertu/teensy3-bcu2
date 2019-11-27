//
// Environment Handling
//
// © Copyright 2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

extern void env_init(void);
extern const char *env_get(const char *key);
extern void env_set(const char *key, const char *val);
extern void env_print(void);
extern void env_save(void);
