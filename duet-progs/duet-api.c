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
#include "ioctl.h"

#define DUET_DEV_NAME   "/dev/duet"		

int open_duet_dev(void)		
{		
	int ret;		
	struct stat st;		

	ret = stat(DUET_DEV_NAME, &st);		
	if (ret < 0) {		
		ret = system("modprobe duet");		
		if (ret == -1)		
			return -1;		
		
		ret = stat(DUET_DEV_NAME, &st);		
		if (ret < 0)		
			return -1;		
	}		
		
	if (S_ISCHR(st.st_mode))		
		duet_fd = open(DUET_DEV_NAME, O_RDWR);		
		
	return duet_fd;		
}		
		
void close_duet_dev(void)
{		
	int ret;		
	struct stat st;		
		
	ret = stat(DUET_DEV_NAME, &st);		
	if (ret < 0)		
		return;		
		
	close(duet_fd);		
}

int duet_register(const char *name, __u32 regmask, const char *path)
{
	int ret = 0;
	struct duet_ioctl_init_args args;

	args.size = sizeof(args);
	memcpy(args.name, name, NAME_MAX);
	args.regmask = regmask;
	memcpy(args.path, path, PATH_MAX);

	ret = ioctl(duet_fd, DUET_IOC_INIT, &args);
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
	struct duet_ioctl_bmap_args args;

	args.size = sizeof(args);
	args.flags = DUET_BMAP_SET;
	args.uuid = uuid;

	ret = ioctl(duet_fd, DUET_IOC_BMAP, &args);
	if (ret < 0)
		perror("duet_set_done: syscall failed");

	if (!ret)
		duet_dbg("Added (ino%lu, gen%u) to task %d\n",
			args.uuid.ino, args.uuid.gen, args.uuid.tid);

	return ret;
}

int duet_reset_done(struct duet_uuid uuid)
{
	int ret = 0;
	struct duet_ioctl_bmap_args args;

	args.size = sizeof(args);
	args.flags = DUET_BMAP_RESET;
	args.uuid = uuid;

	ret = ioctl(duet_fd, DUET_IOC_BMAP, &args);
	if (ret < 0)
		perror("duet_reset_done: syscall failed");

	if (!ret)
		duet_dbg("Removed (ino%lu, gen%u) to task %d\n",
			args.uuid.ino, args.uuid.gen, args.uuid.tid);

	return ret;
}

int duet_check_done(struct duet_uuid uuid)
{
	int ret = 0;
	struct duet_ioctl_bmap_args args;

	args.size = sizeof(args);
	args.flags = DUET_BMAP_CHECK;
	args.uuid = uuid;

	ret = ioctl(duet_fd, DUET_IOC_BMAP, &args);
	if (ret < 0)
		perror("duet_check_done: syscall failed");

	if (ret >= 0)
		duet_dbg("(ino%lu, gen%u) in task %d is %sset (ret%d)\n",
			args.uuid.ino, args.uuid.gen, args.uuid.tid,
			(ret == 1) ? "" : "not ", ret);

	return ret;
}

/* Caller needs to free returned path */
char *duet_get_path(struct duet_uuid uuid)
{
	int ret;
	char *path;
	struct duet_ioctl_gpath_args args;

	path = calloc(PATH_MAX, sizeof(char));
	if (!path)
		return path;

	args.size = sizeof(args);
	args.uuid = uuid;
	args.path[0] = '\0';

	ret = ioctl(duet_fd, DUET_IOC_GPATH, &args);
	if (ret < 0)
		perror("duet_get_path: syscall failed");

	if (!ret)
		duet_dbg("(ino%lu, gen%u) was matched to path %s\n",
			args.uuid.ino, args.uuid.gen, args.path);

	if (ret)
		memcpy(path, args.path, PATH_MAX);

	return path;
}

int duet_print_bmap(int id)
{
	int ret = 0;
	struct duet_ioctl_status_args args;

	if (id <= 0) {
		fprintf(stderr, "duet_print_bmap: invalid id\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.flags = DUET_STATUS_PRINT_BMAP;
	args.id = id;

	ret = ioctl(duet_fd, DUET_IOC_STATUS, &args);
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
	struct duet_ioctl_status_args args;

	if (id <= 0) {
		fprintf(stderr, "duet_print_item: invalid id\n");
		return 1;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);
	args.flags = DUET_STATUS_PRINT_ITEM;
	args.id = id;

	ret = ioctl(duet_fd, DUET_IOC_STATUS, &args);
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
	struct duet_ioctl_status_args *args;
	size_t args_size;

	if (numtasks <= 0 || numtasks > 65535) {
		fprintf(stderr, "duet_print_list: invalid number of tasks\n");
		return 1;
	}

	args_size = sizeof(struct duet_ioctl_status_args) + (numtasks *
		    sizeof(struct duet_task_attrs));
	args = malloc(args_size);
	if (!args) {
		perror("duet_print_list: task list allocation failed");
		return 1;
	}

	memset(args, 0, args_size);
	args->size = sizeof(*args);
	args->flags = DUET_STATUS_PRINT_LIST;
	args->numtasks = numtasks;

	ret = ioctl(duet_fd, DUET_IOC_STATUS, args);
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
