/*
 * Copyright (C) 2014-2015 George Amvrosiadis.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "duet.h"

#define ARGV0_BUF_SIZE	64

struct cmd_struct {
	const char *token;
	int (*fn)(int, char **);

	/*
	 * Usage strings
	 *
	 * A NULL-terminated array of the following format:
	 *
	 *   usagestr[0] - one-line synopsis (required)
	 *   usagestr[1] - one-line short description (required)
	 *   usagestr[2..m] - a long (possibly multi-line) description
	 *                    (optional)
	 *   usagestr[m + 1] - an empty line separator (required if at least one
	 *                     option string is given, not needed otherwise)
	 *   usagestr[m + 2..n] - option strings, one option per line
	 *                        (optional)
	 *   usagestr[n + 1] - NULL terminator
	 *
	 * Options (if present) should always (even if there is no long
	 * description) be prepended with an empty line.  Supplied strings are
	 * indented but otherwise printed as-is, no automatic wrapping is done.
	 *
	 * Grep for cmd_*_usage[] for examples.
	 */
	const char * const *usagestr;

	/* should be NULL if token is not a subgroup */
	const struct cmd_group *next;

	/* if true don't list this token in help listings */
	int hidden;
};

#define NULL_CMD_STRUCT {NULL, NULL, NULL, NULL, 0}

struct cmd_group {
	const char * const *usagestr;
	const char *infostr;
	const struct cmd_struct commands[];
};

/* duet.c */
int prefixcmp(const char *str, const char *prefix);
int check_argc_exact(int nargs, int expected);
int handle_command_group(const struct cmd_group *grp, int argc, char **argv);

/* help.c */
void usage(const char * const *usagestr) __attribute__((noreturn));
void usage_command(const struct cmd_struct *cmd, int full, int err);
void usage_command_group(const struct cmd_group *grp, int all, int err);

void help_unknown_token(const char *arg, const struct cmd_group *grp) __attribute__((noreturn));
void help_ambiguous_token(const char *arg, const struct cmd_group *grp) __attribute__((noreturn));
void help_command_group(const struct cmd_group *grp, int argc, char **argv);

/* Command groups */
extern const struct cmd_group status_cmd_group;
extern const struct cmd_group task_cmd_group;
extern const struct cmd_group debug_cmd_group;

/* Command usage strings */
extern const char * const cmd_status_usage[];
extern const char * const cmd_task_usage[];
extern const char * const cmd_debug_usage[];

/* Command handlers */
int cmd_status(int argc, char **argv);
int cmd_debug(int argc, char **argv);
