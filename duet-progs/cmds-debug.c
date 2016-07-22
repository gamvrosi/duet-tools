/*
 * Copyright (C) 2014 George Amvrosiadis.  All rights reserved.
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

#include "syscall.h"
#include "commands.h"

static const char * const debug_cmd_group_usage[] = {
	"duet debug <command> [options]",
	NULL
};

static const char * const cmd_debug_print_bmap_usage[] = {
	"duet debug bmap [-f fd]",
	"Prints the BitTree for a task.",
	"Instructs the framework to print the BitTree for the given task.",
	"",
	"-f     the fd of the task",
	NULL
};

static const char * const cmd_debug_print_item_usage[] = {
	"duet debug item [-f fd]",
	"Prints the ItemTree for a task.",
	"Instructs the framework to print the ItemTree for the given task.",
	"",
	"-f     the fd of the task",
	NULL
};

static const char * const cmd_debug_print_list_usage[] = {
	"duet debug list [-n num]",
	"List tasks registered with the duet framework.",
	"Requests and prints a list of all the tasks that are currently",
	"registered with the duet framework. For each task, we print the",
	"information maintained by the framework.",
	"",
	"-n	number of tasks to get (default: 32)",
	NULL
};

#if 0
static const char * const cmd_debug_getpath_usage[] = {
	"duet debug getpath [tid] [child uuid]",
	"Check that [child uuid] falls under the namespace subtree the task has",
	"registered for, which is expected to be dir. The tid is necessary",
	"to know which task is requesting this mapping, and which superblock",
	"and namespace we're referring to.",
	"",
	NULL
};
#endif /* 0 */

static int cmd_debug_print_bmap(int argc, char **argv)
{
	int c, ret=0;
	struct duet_status_args args;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	optind = 1;
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			errno = 0;
			args.fd = (__u8)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid ID");
				usage(cmd_debug_print_bmap_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_debug_print_bmap_usage);
		}
	}

	if (!args.fd || argc != optind)
		usage(cmd_debug_print_bmap_usage);

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_PRINT_BMAP, &args);
	if (ret < 0) {
		perror("duet debug: bitmap printing failed");
		usage(cmd_debug_print_bmap_usage);
	}

	fprintf(stdout, "Check dmesg for the BitTree of task %d.\n",
		args.fd);
	return ret;
}

static int cmd_debug_print_item(int argc, char **argv)
{
	int c, ret=0;
	struct duet_status_args args;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	optind = 1;
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			errno = 0;
			args.fd = (__u8)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid ID");
				usage(cmd_debug_print_item_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_debug_print_item_usage);
		}
	}

	if (!args.fd || argc != optind)
		usage(cmd_debug_print_item_usage);

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_PRINT_ITEM, &args);
	if (ret < 0) {
		perror("duet debug: item printing failed");
		usage(cmd_debug_print_item_usage);
	}

	fprintf(stdout, "Check dmesg for the ItemTree of task %d.\n",
		args.fd);
	return ret;
}

static int cmd_debug_print_list(int argc, char **argv)
{
	int c, numtasks = 32, ret = 0;

	optind = 1;
	while ((c = getopt(argc, argv, "n:")) != -1) {
		switch (c) {
		case 'n':
			errno = 0;
			numtasks = (int)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid task number");
				usage(cmd_debug_print_list_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_debug_print_list_usage);
		}
	}

	if (numtasks <= 0 || numtasks > 65535 || argc != optind)
		usage(cmd_debug_print_list_usage);

	ret = duet_print_list(numtasks);
	if (ret < 0) {
		perror("duet debug: list printing failed");
		usage(cmd_debug_print_list_usage);
	}

	return ret;
}

#if 0
static int cmd_debug_getpath(int fd, int argc, char **argv)
{
	int ret=0;
	struct duet_ioctl_cmd_args args;

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_GET_PATH;

	if (argc != 3)
		usage(cmd_debug_getpath_usage);

	/* Pass the inode numbers in */
	errno = 0;
	args.tid = (__u8)strtoul(argv[1], NULL, 10);
	if (errno) {
		perror("strtol: invalid task ID");
		usage(cmd_debug_printbit_usage);
	}

	errno = 0;
	args.c_uuid = (unsigned long long)strtoull(argv[2], NULL, 16);
	if (errno) {
		perror("strtoll: invalid child uuid");
		usage(cmd_debug_getpath_usage);
	}

	ret = ioctl(fd, DUET_IOC_CMD, &args);
	if (ret < 0) {
		perror("debug isparent ioctl error");
		usage(cmd_debug_getpath_usage);
	}

	fprintf(stdout, "%llu is %spart of the namespace (%s)\n", args.c_uuid,
		args.cpath[0] == '\0' ? "not " : "",
		args.cpath[0] == '\0' ? "" : args.cpath);
	return ret;
}
#endif /* 0 */

const struct cmd_group debug_cmd_group = {
	debug_cmd_group_usage, NULL, {
		{ "bmap", cmd_debug_print_bmap, cmd_debug_print_bmap_usage, NULL, 0 },
		{ "item", cmd_debug_print_item, cmd_debug_print_item_usage, NULL, 0 },
		{ "list", cmd_debug_print_list, cmd_debug_print_list_usage, NULL, 0 },
		//{ "getpath", cmd_debug_getpath, cmd_debug_getpath_usage, NULL, 0 },
	}
};

int cmd_debug(int argc, char **argv)
{
	return handle_command_group(&debug_cmd_group, argc, argv);
}
