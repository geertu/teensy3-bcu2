//
// Task Management
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

#include "print.h"
#include "task.h"

static struct task *tasks;

void task_add(struct task *task)
{
	task->base = micros() - task->period;
	task->prev = NULL;
	task->next = tasks;
	if (tasks)
		tasks->prev = task;
	tasks = task;
}

void task_del(struct task *task)
{
	if (task->prev)
		task->prev->next = task->next;
	else
		tasks = task->next;
	if (task->next)
		task->next->prev = task->prev;
	task->prev = NULL;
	task->next = NULL;
}

static void task_run(struct task *task)
{
	struct task *prev, *next;
	uint32_t now;
	int error;

	pr_debug("Running task %s\n", task->name);
	/* Run task */
	error = task->func();

	pr_debug("Removing task %s from list\n", task->name);
	/* Remove (always first) task from list */
	task_del(task);

	pr_debug("Checking for completion of task %s\n", task->name);
	if (error) {
		pr_info("Task %s stopped with error %d\n", task->name, error);
		return;
	}

	task->base += task->period;
	pr_debug("Next run of task %s at %lu\n", task->name,
		 task->base + task->period);

	/* Enqueue task in list */
	now = micros();
	for (prev = NULL, next = tasks; next; prev = next, next = next->next) {
		if ((int32_t)(task->base + task->period - now) <
		    (int32_t)(next->base + next->period - now))
			break;
	}
	task->prev = prev;
	task->next = next;
	if (prev)
		prev->next = task;
	else
		tasks = task;
	if (next)
		next->prev = task;
}

void task_run_loop(void)
{
	struct task *task;
	uint32_t now;

	while (tasks) {
		task = tasks;

		pr_debug("Waiting for %lu to run task %s\n",
			 task->base + task->period, task->name);
		while (1) {
			now = micros();
			if (now - task->base >= task->period)
				break;

			delay(10);
		}
		task_run(task);
	}
	pr_err("PANIC: No more tasks to run\n");
}
