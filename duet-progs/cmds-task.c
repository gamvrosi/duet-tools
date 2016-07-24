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

#include "commands.h"
#include "syscall.h"

#if 0
static const char * const cmd_task_mark_usage[] = {
	"duet task mark [-i id] [-o offset] [-l len]",
	"Marks a block range for a specific task.",
	"Finds and marks the given block range (in bytes), in the bitmaps",
	"of the task with the given id.",
	"",
	"-i     the id of the task",
	"-o     the offset denoting the beginning of the range in bytes",
	"-l     the number of bytes denoting the length of the range",
	NULL
};

static const char * const cmd_task_unmark_usage[] = {
	"duet task unmark [-i id] [-o offset] [-l len]",
	"Unmarks a block range for a specific task.",
	"Finds and unmarks the given block range (in bytes), in the bitmaps",
	"of the task wit the given id.",
	"",
	"-i     the id of the task",
	"-o     the offset denoting the beginning of the range in bytes",
	"-l     the number of bytes denoting the length of the range",
	NULL
};

static const char * const cmd_task_check_usage[] = {
	"duet task check [-i id] [-o offset] [-l len]",
	"Checks if a block range for a specific task is marked or not.",
	"Finds and checks if the given block range (in bytes) is marked or not",
	"in the bitmaps of the task wit the given id.",
	"",
	"-i     the id of the task",
	"-o     the offset denoting the beginning of the range in bytes",
	"-l     the number of bytes denoting the length of the range",
	NULL
};

static const char * const cmd_debug_getpath_usage[] = {
	"duet debug getpath [tid] [child uuid]",
	"Check that [child uuid] falls under the namespace subtree the task has",
	"registered for, which is expected to be dir. The tid is necessary",
	"to know which task is requesting this mapping, and which superblock",
	"and namespace we're referring to.",
	"",
	NULL
};

static int cmd_task_mark(int fd, int argc, char **argv)
{
	int c, tid = 0, ret = 0;
	__u64 idx = 0;
	__u32 count = 0;

	optind = 1;
	while ((c = getopt(argc, argv, "i:o:l:")) != -1) {
		switch (c) {
		case 'i':
			errno = 0;
			tid = (__u8)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid ID");
				usage(cmd_task_mark_usage);
			}
			break;
		case 'o':
			errno = 0;
			idx = (__u64)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtoll: invalid offset");
				usage(cmd_task_mark_usage);
			}
			break;
		case 'l':
			errno = 0;
			count = (__u32)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid length");
				usage(cmd_task_mark_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_task_mark_usage);
		}
	}

	if (!tid || !count || argc != optind)
		usage(cmd_task_mark_usage);

	ret = duet_set_done(fd, tid, idx, count);
	if (ret < 0) {
		perror("debug addblk ioctl error");
		usage(cmd_task_mark_usage);
	}

	fprintf(stdout, "Success adding blocks [%llu, %llu] to task #%d.\n",
		idx, idx + count, tid);
	return ret;
}

static int cmd_task_unmark(int fd, int argc, char **argv)
{
	int c, tid = 0, ret = 0;
	__u64 idx = 0;
	__u32 count = 0;

	optind = 1;
	while ((c = getopt(argc, argv, "i:o:l:")) != -1) {
		switch (c) {
		case 'i':
			errno = 0;
			tid = (__u8)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid ID");
				usage(cmd_task_unmark_usage);
			}
			break;
		case 'o':
			errno = 0;
			idx = (__u64)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtoll: invalid offset");
				usage(cmd_task_unmark_usage);
			}
			break;
		case 'l':
			errno = 0;
			count = (__u32)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid length");
				usage(cmd_task_unmark_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_task_unmark_usage);
		}
	}

	if (!tid || !count || argc != optind)
		usage(cmd_task_unmark_usage);

	ret = duet_unset_done(fd, tid, idx, count);
	if (ret < 0) {
		perror("debug rmblk ioctl error");
		usage(cmd_task_unmark_usage);
	}

	fprintf(stdout, "Success removing blocks [%llu, %llu] to task #%d.\n",
		idx, idx + count, tid);
	return ret;
}

static int cmd_task_check(int fd, int argc, char **argv)
{
	int c, tid = 0, ret = 0;
	__u64 idx = 0;
	__u32 count = 0;

	optind = 1;
	while ((c = getopt(argc, argv, "i:o:l:")) != -1) {
		switch (c) {
		case 'i':
			errno = 0;
			tid = (__u8)strtol(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid ID");
				usage(cmd_task_check_usage);
			}
			break;
		case 'o':
			errno = 0;
			idx = (__u64)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtoll: invalid offset");
				usage(cmd_task_check_usage);
			}
			break;
		case 'l':
			errno = 0;
			count = (__u32)strtoll(optarg, NULL, 10);
			if (errno) {
				perror("strtol: invalid length");
				usage(cmd_task_check_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_task_check_usage);
		}
	}

	if (!tid || !count || argc != optind)
		usage(cmd_task_check_usage);

	ret = duet_check_done(fd, tid, idx, count);
	if (ret < 0) {
		perror("debug chkblk ioctl error");
		usage(cmd_task_check_usage);
	}

	fprintf(stdout, "Blocks [%llu, %llu] in task #%d were %sset.\n",
		idx, idx + count, tid, ret ? "" : "not ");
	return 0;
}

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
