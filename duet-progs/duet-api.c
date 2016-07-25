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

int duet_register(const char *name, __u32 regmask, const char *path)
{
	int ret = 0;

	/* Call syscall x86_64 #330: duet_init */
	ret = syscall(330, name, regmask, path);
	if (ret < 0)
		perror("duet_register: syscall failed");

	if (!ret)
		duet_dbg(stdout, "Error registering task %s.\n", name);
	else
		duet_dbg(stdout, "Successfully registered task %s.\n", name);

	return ret;
}

int duet_set_done(struct duet_uuid uuid)
{
	int ret = 0;
	struct duet_uuid_arg arg;

	arg.size = sizeof(arg);
	arg.uuid = uuid;

	/* Call syscall x86_64 #331: duet_bmap */
	ret = syscall(331, DUET_BMAP_SET, &arg);
	if (ret < 0)
		perror("duet_set_done: syscall failed");

	if (!ret)
		duet_dbg("Added (ino%lu, gen%u) to task %d\n",
			arg.uuid.ino, arg.uuid.gen, arg.uuid.tid);

	return ret;
}

int duet_reset_done(struct duet_uuid uuid)
{
	int ret = 0;
	struct duet_uuid_arg arg;

	arg.size = sizeof(arg);
	arg.uuid = uuid;

	/* Call syscall x86_64 #331: duet_bmap */
	ret = syscall(331, DUET_BMAP_RESET, &arg);
	if (ret < 0)
		perror("duet_reset_done: syscall failed");

	if (!ret)
		duet_dbg("Removed (ino%lu, gen%u) to task %d\n",
			arg.uuid.ino, arg.uuid.gen, arg.uuid.tid);

	return ret;
}

int duet_check_done(struct duet_uuid uuid)
{
	int ret = 0;
	struct duet_uuid_arg arg;

	arg.size = sizeof(arg);
	arg.uuid = uuid;

	/* Call syscall x86_64 #331: duet_bmap */
	ret = syscall(331, DUET_BMAP_CHECK, &arg);
	if (ret < 0)
		perror("duet_check_done: syscall failed");

	if (ret >= 0)
		duet_dbg("(ino%lu, gen%u) in task %d is %sset (ret%d)\n",
			arg.uuid.ino, arg.uuid.gen, arg.uuid.tid,
			(ret == 1) ? "" : "not ", ret);

	return ret;
}

#if 0
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

int duet_print_bmap(int id)
{
	int ret = 0;
	struct duet_status_args args;

	if (id <= 0) {
		fprintf(stderr, "duet_print_bmap: invalid id\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.id = id;

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_STATUS_PRINT_BMAP, &args);
	if (ret < 0) {
		perror("duet_print_bmap: syscall failed");
		return ret;
	}

	fprintf(stdout, "Check dmesg for the BitTree of task %d\n", args.id);

	return ret;
}

int duet_print_item(int id)
{
	int ret = 0;
	struct duet_status_args args;

	if (id <= 0) {
		fprintf(stderr, "duet_print_item: invalid id\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.id = id;

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_STATUS_PRINT_ITEM, &args);
	if (ret < 0) {
		perror("duet_print_item: syscall failed");
		return ret;
	}

	fprintf(stdout, "check dmesg for the ItemTree of task %d\n", args.id);

	return ret;
}

#define PRINT_MASK(mask) \
	fprintf(stdout, "Reg. mask: %s%s%s%s%s%s%s\n", \
		((mask & DUET_PAGE_ADDED) ? "ADDED " : ""), \
		((mask & DUET_PAGE_REMOVED) ? "REMOVED " : ""), \
		((mask & DUET_PAGE_DIRTY) ? "DIRTY " : ""), \
		((mask & DUET_PAGE_FLUSHED) ? "FLUSHED " : ""), \
		((mask & DUET_PAGE_EXISTS) ? "EXISTS " : ""), \
		((mask & DUET_PAGE_MODIFIED) ? "MODIFIED " : ""), \
		((mask & DUET_FD_NONBLOCK) ? "O_NONBLOCK " : ""));

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
	ret = syscall(329, DUET_STATUS_PRINT_LIST, args);
	if (ret < 0) {
		perror("duet_print_list: syscall failed");
		goto out;
	}

	/* Print out the list we received */
	fprintf(stdout, "Duet task listing:\n\n");

	for (i = 0; i < args->numtasks; i++) {
		if (!args->tasks[i].id)
			break;

		fprintf(stdout, "Task Id: %d\n", args->tasks[i].id);
		fprintf(stdout, "Task fd: %d\n", args->tasks[i].fd);
		fprintf(stdout, "Task name: %s\n", args->tasks[i].name);
		PRINT_MASK(args->tasks[i].regmask);
		fprintf(stdout, "Reg. path: %s\n\n", args->tasks[i].path);
	}

	fprintf(stdout, "Listed %d tasks.\n", i);

out:
	free(args);
	return ret;
}
