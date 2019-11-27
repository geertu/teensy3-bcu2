//
// Command Handling
//
// © Copyright 2019-2020 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License, version 2.
//

enum cmd_mode {
	CMD_COMMAND,
	CMD_MONITOR,
	CMD_TEST,
};

extern int cmd_mode;

extern void cmd_run(char *line);
extern void cmd_prompt(void);
