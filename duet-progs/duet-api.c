/*
 * Copyright (C) 2015 George Amvrosiadis.  All rights reserved.
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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "syscall.h"

#if 0
int duet_register(int duet_fd, const char *path, __u32 regmask, __u32 bitrange,
	const char *name, int *tid)
{
	int ret = 0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));

	args.cmd_flags = DUET_REGISTER;
	memcpy(args.name, name, DUET_MAX_NAME);
	args.bitrange = bitrange;
	args.regmask = regmask;
	memcpy(args.path, path, DUET_MAX_PATH);

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0)
		perror("duet: tasks register ioctl error");

	*tid = args.tid;

	if (args.ret)
		duet_dbg(stdout, "Error registering task (ID %d).\n", args.tid);
	else
		duet_dbg(stdout, "Successfully registered task (ID %d).\n", args.tid);

	return (ret < 0) ? ret : args.ret;
}

int duet_deregister(int duet_fd, int tid)
{
	int ret = 0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_DEREGISTER;
	args.tid = tid;

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0)
		perror("duet: tasks deregister ioctl error");

	if (args.ret)
		duet_dbg(stdout, "Error deregistering task (ID %d).\n", args.tid);
	else
		duet_dbg(stdout, "Successfully deregistered task (ID %d).\n", args.tid);

	return (ret < 0) ? ret : args.ret;
}

int duet_fetch(int duet_fd, int tid, struct duet_item *items, int *count)
{
	int ret = 0;
	struct duet_ioctl_fetch_args *args;
	size_t args_size;

	if (*count > 65535) {
		fprintf(stderr, "duet: requested too many items (%d > 65535)\n",
			*count);
		return -1;
	}

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	args_size = sizeof(*args) + (*count * sizeof(struct duet_item));
	args = malloc(args_size);
	if (!args) {
		perror("duet: fetch args allocation failed");
		return 1;
	}

	memset(args, 0, args_size);
	args->tid = tid;
	args->num = *count;

	ret = ioctl(duet_fd, DUET_IOC_FETCH, args);
	if (ret < 0) {
		//perror("duet: fetch ioctl error");
		goto out;
	}

	/* Copy out results */
	*count = args->num;
	memcpy(items, args->itm, *count * sizeof(struct duet_item));

out:
	free(args);
	return ret;
}

int duet_check_done(int duet_fd, int tid, __u64 idx, __u32 count)
{
	int ret = 0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_CHECK_DONE;
	args.tid = tid;
	args.itmidx = idx;
	args.itmnum = count;

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0)
		perror("duet: check ioctl error");

	duet_dbg("Blocks [%llu, %llu] in task #%d were %sset.\n",
		args.itmidx, args.itmidx + args.itmnum, args.tid,
		args.ret ? "" : "not ");

	return (ret < 0) ? ret : args.ret;
}

int duet_set_done(int duet_fd, int tid, __u64 idx, __u32 count)
{
	int ret = 0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_SET_DONE;
	args.tid = tid;
	args.itmidx = idx;
	args.itmnum = count;

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0)
		perror("duet: mark ioctl error");

	duet_dbg("Added blocks [%llu, %llu] to task #%d (ret = %u)\n",
		args.itmidx, args.itmidx + args.itmnum, args.tid, args.ret);

	return (ret < 0) ? ret : args.ret;
}

int duet_unset_done(int duet_fd, int tid, __u64 idx, __u32 count)
{
	int ret = 0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return -1;
	}

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_UNSET_DONE;
	args.tid = tid;
	args.itmidx = idx;
	args.itmnum = count;

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0)
		perror("duet: unmark ioctl error");

	duet_dbg("Removed blocks [%llu, %llu] to task #%d (ret = %u).\n",
		args.itmidx, args.itmidx + args.itmnum, args.tid, args.ret);

	return (ret < 0) ? ret : args.ret;
}

/* Warning: should only be called with a path that's DUET_MAX_PATH or longer */
int duet_get_path(int duet_fd, int tid, unsigned long long uuid, char *path)
{
	int ret=0;
	struct duet_ioctl_cmd_args args;

	if (duet_fd == -1) {
		fprintf(stderr, "duet: failed to open duet device\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.cmd_flags = DUET_GET_PATH;
	args.tid = tid;
	args.c_uuid = uuid;

	ret = ioctl(duet_fd, DUET_IOC_CMD, &args);
	if (ret < 0) {
		perror("duet: getpath ioctl error");
		goto out;
	}

	if (!args.ret)
		memcpy(path, args.cpath, DUET_MAX_PATH);

out:
	return args.ret || ret;
}
#endif /* 0 */

int duet_print_bmap(int fd)
{
	int ret = 0;
	struct duet_status_args args;

	if (fd <= 0) {
		fprintf(stderr, "duet_print_bmap: invalid fd\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.fd = fd;

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_PRINT_BMAP, &args);
	if (ret < 0) {
		perror("duet_print_bmap: syscall failed");
		return ret;
	}

	fprintf(stdout, "Check dmesg for the BitTree of task %d\n", args.fd);

	return ret;
}

int duet_print_item(int fd)
{
	int ret = 0;
	struct duet_status_args args;

	if (fd <= 0) {
		fprintf(stderr, "duet_print_item: invalid fd\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.fd = fd;

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_PRINT_ITEM, &args);
	if (ret < 0) {
		perror("duet_print_item: syscall failed");
		return ret;
	}

	fprintf(stdout, "check dmesg for the ItemTree of task %d\n", args.fd);

	return ret;
}

int duet_print_list(int numtasks)
{
	int i, ret = 0;
	struct duet_status_args *args;
	size_t args_size;

	if (numtasks <= 0 || numtasks > 65535) {
		fprintf(stderr, "duet_print_list: invalid number of tasks\n");
		return 1;
	}

	args_size = sizeof(struct duet_status_args) + (numtasks *
		    sizeof(struct duet_task_attrs));
	args = malloc(args_size);
	if (!args) {
		perror("duet_print_list: task list allocation failed");
		return 1;
	}

	memset(args, 0, args_size);
	args->size = sizeof(*args);
	args->numtasks = numtasks;

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_PRINT_LIST, args);
	if (ret < 0) {
		perror("duet_print_list: syscall failed");
		goto out;
	}

	/* Print out the list we received */
	fprintf(stdout,
		"fd   \tTask Name           \tFile task?\tReg. mask\tReg. path\n"
		"-----\t--------------------\t----------\t---------\t---------\n");
	for (i = 0; i < args->numtasks; i++) {
		if (!args->tasks[i].fd)
			break;

		fprintf(stdout, "%5d\t%20s\t%10s\t%9x\t%s\n",
			args->tasks[i].fd, args->tasks[i].name,
			args->tasks[i].is_file ? "TRUE" : "FALSE",
			args->tasks[i].regmask, args->tasks[i].path);
	}

out:
	free(args);
	return ret;
}
