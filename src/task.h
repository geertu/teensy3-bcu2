//
// Task Management
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#define HZ			1000000

struct task {
	const char *name;
	int (*func)(void);
	unsigned int period;	// us
	/* private */
	uint32_t base;
	struct task *prev;
	struct task *next;
};

extern void task_add(struct task *task);
extern void task_del(struct task *task);
extern void task_run_loop(void);
