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

#include "commands.h"
#include "syscall.h"

#define DSH_TOK_BUFSZ	64
#define DSH_TOK_DELIM	" \t\r\n\a"
#define DSH_MAX_FDS	256

int duet_fds[DSH_MAX_FDS];
int fdnum;

const char * const cmd_debug_usage[] = {
	"duet debug",
	"Spawns an interactive shell for Duet debugging.",
	"To see available commands, type 'help' at the shell.",
	NULL
};

/* Shell commands */
int dsh_print(char **argv)
{
	int ret = 0;
	struct duet_status_args args;

	if (!argv[1] || !argv[2]) {
		fprintf(stderr, "Error: Invalid print args\n");
		return 0;
	}

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	if (!strcmp(argv[1], "bmap")) {
		errno = 0;
		args.id = (__u8)strtol(argv[2], NULL, 10);
		if (errno || !args.id) {
			perror("print bmap: invalid fd");
			return 0;	
		}
		
		/* Call syscall x86_64 #329: duet_status */
		ret = syscall(329, DUET_STATUS_PRINT_BMAP, &args);
		if (ret < 0) {
			perror("print bmap: syscall failed");
			return 0;
		}

		fprintf(stdout, "Check dmesg for task BitTree.\n");

	} else if (!strcmp(argv[1], "item")) {
		errno = 0;
		args.id = (__u8)strtol(argv[2], NULL, 10);
		if (errno || !args.id) {
			perror("print item: invalid fd");
			return 0;
		}

		/* Call syscall x86_64 #329: duet_status */
		ret = syscall(329, DUET_STATUS_PRINT_ITEM, &args);
		if (ret < 0) {
			perror("print item: syscall failed");
			return 0;
		}

		fprintf(stdout, "Check dmesg for task ItemTree.\n");

	} else {
		fprintf(stderr, "Error: print %s is not a valid command\n",
			argv[1]);
		return 0;
	}

	return 0;
}

int dsh_list(char **argv)
{
	int numtasks = 32, ret = 0;

	if (argv[1]) {
		errno = 0;
		numtasks = (int)strtol(argv[1], NULL, 10);
		if (errno) {
			perror("list: invalid task number");
			return 0;
		}
	}

	ret = duet_print_list(numtasks);
	if (ret < 0) {
		perror("list: printing failed");
		return 0;
	}

	return 0;
}

int dsh_reg(char **argv)
{
	int len = 0, ret = 0;
	char path[PATH_MAX], name[NAME_MAX];
	__u32 regmask = 0;

	if (!argv[1] || !argv[2] || !argv[3]) {
		fprintf(stderr, "Error: Invalid register args\n");
		return 0;
	}

	/* Make sure there is an available fd slot */
	if (fdnum == DSH_MAX_FDS) {
		fprintf(stderr, "Error: No available fds\n");
		return 0;
	}

	memset(path, 0, PATH_MAX);
	memset(name, 0, NAME_MAX);

	/* Copy the name */
	len = strnlen(argv[1], NAME_MAX);
	if (len >= NAME_MAX || !len) {
		fprintf(stderr, "register: invalid name\n");
		return 0;
	}
	memcpy(name, argv[1], len);

	/* Copy the regmask */
	errno = 0;
	regmask = (__u32)strtol(argv[2], NULL, 16);
	if (errno) {
		perror("register: invalid regmask");
		return 0;
	}

	/* Copy the path */
	len = strnlen(argv[3], PATH_MAX);
	if (len >= PATH_MAX || !len) {
		fprintf(stderr, "register: invalid path\n");
		return 0;
	}
	memcpy(path, argv[3], len);

	ret = duet_register(name, regmask | DUET_FD_NONBLOCK, path);
	if (ret <= 0)
		return 0;

	duet_fds[fdnum] = ret;
	fdnum++;

	fprintf(stdout, "Success registering task '%s' (fd %d)\n", name, ret);
	return 0;
}

int dsh_dereg(char **argv)
{
	int i, fdin;

	if (!argv[1]) {
		fprintf(stderr, "Error: Invalid deregistration args\n");
		return 0;
	}

	/* Copy the fd number */
	errno = 0;
	fdin = (int)strtol(argv[1], NULL, 10);
	if (errno || fdin <= 0) {
		perror("deregister: invalid file descriptor");
		return 0;
	}

	/* Find the fd in the already opened ones */
	if (!fdnum) {
		fprintf(stderr, "Error: No registered tasks\n");
		return 0;
	}

	for (i = 0; i < fdnum; i++) {
		if (duet_fds[i] == fdin) {
			close(fdin);

			fprintf(stdout, "Success deregistered task (fd %d)\n",
				fdin);
			fdin = 0;
			fdnum--;

			/* Fill the hole with the last fd */
			if (fdnum) {
				duet_fds[i] = duet_fds[fdnum];
				duet_fds[fdnum] = 0;
			} 

			return 0;
		}
	}

	fprintf(stderr, "deregister: task not found\n");
	return 0;
}

#define UUID_IDX(uuid)	(((unsigned long long) uuid.gen << 32) | \
			  (unsigned long long) uuid.ino)

#define PRINT_STATE(mask) \
	fprintf(stdout, "    Events: %s%s%s%s\n", \
		((mask & DUET_PAGE_ADDED) ? "ADDED " : ""), \
		((mask & DUET_PAGE_REMOVED) ? "REMOVED " : ""), \
		((mask & DUET_PAGE_DIRTY) ? "DIRTY " : ""), \
		((mask & DUET_PAGE_FLUSHED) ? "FLUSHED " : ""));

int dsh_read(char **argv)
{
	int fdin, num, count;
	char *buf, *pos;
	ssize_t ret;
	size_t bufsize;
	struct duet_item *itm;

	if (!argv[1] || !argv[2]) {
		fprintf(stderr, "Error: Invalid read args\n");
		return 0;
	}

	/* Copy the fd number */
	errno = 0;
	fdin = (int)strtol(argv[1], NULL, 10);
	if (errno || fdin <= 0) {
		perror("read: invalid file descriptor");
		return 0;
	}

	/* Copy the number of items */
	errno = 0;
	num = (int)strtol(argv[2], NULL, 10);
	if (errno || num <= 0) {
		perror("read: invalid number of items");
		return 0;
	}

	bufsize = num * sizeof(struct duet_item);
	buf = malloc(bufsize);
	if (!buf) {
		fprintf(stderr, "read: buffer allocation failed\n");
		return -ENOMEM;
	}

	ret = read(fdin, buf, bufsize);
	if (ret < 0 && errno != EAGAIN) {
		perror("read: read call failed");
		goto out;
	}

	/* Print out the received items */
	if (!ret || errno == EAGAIN) {
		fprintf(stdout, "Received no items.\n");
		goto out;
	}

	/* Print out the list we received */
	fprintf(stdout, "Printing received items.\n\n");
	count = 0;
	for (pos = buf; pos < buf + ret; pos += sizeof(struct duet_item)) {
		itm = (struct duet_item *)pos;

		fprintf(stdout, "    Task ID: %u\n"
				"    Inode no: %lu\n"
				"    Inode gen: %u\n"
				"    UUID_IDX: %llu\n"
				"    Byte offset: %lu\n",
			itm->uuid.tid, itm->uuid.ino, itm->uuid.gen,
			UUID_IDX(itm->uuid), itm->idx << 12);
		PRINT_STATE(itm->state);
		fprintf(stdout, "\n");
		count++;
	}
	fprintf(stdout, "\nPrinted %d items\n", count);

out:
	free(buf);
	return 0;
}

int dsh_set(char **argv)
{
	int ret;
	struct duet_uuid uuid;

	if (!argv[1] || !argv[2] || !argv[3]) {
		fprintf(stderr, "Error: Invalid set args\n");
		return 0;
	}

	/* Copy the task id */
	errno = 0;
	uuid.tid = (__u8)strtol(argv[1], NULL, 10);
	if (errno || !uuid.tid) {
		perror("set: invalid task id");
		return 0;
	}

	/* Copy the inode number */
	errno = 0;
	uuid.ino = (unsigned long)strtol(argv[2], NULL, 10);
	if (errno || !uuid.ino) {
		perror("set: invalid inode number");
		return 0;
	}

	/* Copy the inode generation */
	errno = 0;
	uuid.gen = (__u32)strtol(argv[3], NULL, 10);
	if (errno) {
		perror("set: invalid inode generation");
		return 0;
	}

	ret = duet_set_done(uuid);
	if (ret)
		fprintf(stderr, "set: duet_set_done API call failed\n");

	if (!ret)
		duet_dbg(stdout, "Added (ino%lu, gen%u) to task %d\n",
			uuid.ino, uuid.gen, uuid.tid);

	return 0;
}

int dsh_reset(char **argv)
{
	int ret;
	struct duet_uuid uuid;

	if (!argv[1] || !argv[2] || !argv[3]) {
		fprintf(stderr, "Error: Invalid reset args\n");
		return 0;
	}

	/* Copy the task id */
	errno = 0;
	uuid.tid = (__u8)strtol(argv[1], NULL, 10);
	if (errno || !uuid.tid) {
		perror("reset: invalid task id");
		return 0;
	}

	/* Copy the inode number */
	errno = 0;
	uuid.ino = (unsigned long)strtol(argv[2], NULL, 10);
	if (errno || !uuid.ino) {
		perror("reset: invalid inode number");
		return 0;
	}

	/* Copy the inode generation */
	errno = 0;
	uuid.gen = (__u32)strtol(argv[3], NULL, 10);
	if (errno) {
		perror("reset: invalid inode generation");
		return 0;
	}

	ret = duet_reset_done(uuid);
	if (ret)
		fprintf(stderr, "reset: duet_reset_done API call failed\n");

	if (!ret)
		fprintf(stdout, "Removed (ino%lu, gen%u) to task %d\n",
			uuid.ino, uuid.gen, uuid.tid);

	return 0;
}

int dsh_check(char **argv)
{
	int ret;
	struct duet_uuid uuid;

	if (!argv[1] || !argv[2] || !argv[3]) {
		fprintf(stderr, "Error: Invalid check args\n");
		return 0;
	}

	/* Copy the task id */
	errno = 0;
	uuid.tid = (__u8)strtol(argv[1], NULL, 10);
	if (errno || !uuid.tid) {
		perror("check: invalid task id");
		return 0;
	}

	/* Copy the inode number */
	errno = 0;
	uuid.ino = (unsigned long)strtol(argv[2], NULL, 10);
	if (errno || !uuid.ino) {
		perror("check: invalid inode number");
		return 0;
	}

	/* Copy the inode generation */
	errno = 0;
	uuid.gen = (__u32)strtol(argv[3], NULL, 10);
	if (errno) {
		perror("check: invalid inode generation");
		return 0;
	}

	ret = duet_check_done(uuid);
	if (ret < 0)
		fprintf(stderr, "check: duet_check_done API call failed\n");
	else
		fprintf(stdout, "(ino%lu, gen%u) in task %d is %sset (ret%d)\n",
			uuid.ino, uuid.gen, uuid.tid,
			(ret == 1) ? "" : "not ", ret);

	return 0;
}

int dsh_getpath(char **argv)
{
	char *path;
	struct duet_uuid uuid;

	if (!argv[1] || !argv[2] || !argv[3]) {
		fprintf(stderr, "Error: Invalid getpath args\n");
		return 0;
	}

	/* Copy the task id */
	errno = 0;
	uuid.tid = (__u8)strtol(argv[1], NULL, 10);
	if (errno || !uuid.tid) {
		perror("getpath: invalid task id");
		return 0;
	}

	/* Copy the inode number */
	errno = 0;
	uuid.ino = (unsigned long)strtol(argv[2], NULL, 10);
	if (errno || !uuid.ino) {
		perror("getpath: invalid inode number");
		return 0;
	}

	/* Copy the inode generation */
	errno = 0;
	uuid.gen = (__u32)strtol(argv[3], NULL, 10);
	if (errno) {
		perror("getpath: invalid inode generation");
		return 0;
	}

	path = duet_get_path(uuid);
	if (!path)
		fprintf(stdout, "Got no path for (ino%lu, gen%u)\n",
			uuid.ino, uuid.gen);
	if (path)
		fprintf(stdout, "(ino%lu, gen%u) maps to path: %s\n",
			uuid.ino, uuid.gen, path);

	free(path);
	return 0;
}

int dsh_help(char **argv)
{
	fprintf(stdout,
"\n"
"Duet Debugging Shell Help\n"
"~~~~~~~~~~~~~~~~~~~~~~~~~\n"
"\n"
"List of available commands:\n"
"\n"
"    print bmap ID\n"
"        Instructs Duet to print the progress bitmap for a task.\n"
"\n"
"        ID    the unique id of the task\n"
"\n"
"    print item ID\n"
"        Instructs Duet to print the item bitmap for a task.\n"
"\n"
"        ID    the unique id of the task\n"
"\n"
"    list NUM\n"
"        Requests and prints the information of tasks that are\n"
"        currently registered with the framework.\n"
"\n"
"        NUM     the number of tasks to get (default: 32)\n"
"\n"
"    register NAME MASK PATH\n"
"        Registers a new task with Duet.\n"
"\n"
"        NAME    a human-readable name for the task\n"
"        MASK    the mask of events of interest to the task\n"
"        PATH    the path that Duet should watch for events\n"
"\n"
"    deregister FD\n"
"        Deregisters a task previously registered with Duet.\n"
"\n"
"        FD      File descriptor of task (find using 'list')\n"
"\n"
"    read FD NUM\n"
"        Read and pretty print events for a registered task.\n"
"\n"
"        FD      File descriptor of task (find using 'list')\n"
"        NUM     Maximum number of items to read\n"
"\n"
"    set TID INO GEN\n"
"        Set a specific bit on the BitTree of a given task.\n"
"\n"
"        TID     Task ID (find using 'list')\n"
"        INO     Inode number to set\n"
"        GEN     Inode generation to set (find using 'read')\n"
"\n"
"    reset TID INO GEN\n"
"        Reset a specific bit on the BitTree of a given task.\n"
"\n"
"        TID     Task ID (find using 'list')\n"
"        INO     Inode number to reset\n"
"        GEN     Inode generation to reset (find using 'read')\n"
"\n"
"    check TID INO GEN\n"
"        Check whether a specific bit on the BitTree of a given\n"
"        task is set.\n"
"\n"
"        TID     Task ID (find using 'list')\n"
"        INO     Inode number to check\n"
"        GEN     Inode generation to check (find using 'read')\n"
"\n"
"    getpath TID INO GEN\n"
"        Return the path corresponding to the specified inode,\n"
"        if it falls under the task's registered directory.\n"
"\n"
"        TID     Task ID (find using 'list')\n"
"        INO     Inode number to get the path of\n"
"        GEN     Inode generation to get the path of (find using 'read')\n"
"\n");

	return 0;
}

int dsh_exit(char **argv)
{
	return 1;
}

/* Shell boilerplate */
char *dsh_cmd_str[] = {
	"print",
	"list",
	"register",
	"deregister",
	"read",
	"set",
	"reset",
	"check",
	"getpath",
	"help",
	"exit"
};

int (*dsh_cmd_func[]) (char **) = {
	&dsh_print,
	&dsh_list,
	&dsh_reg,
	&dsh_dereg,
	&dsh_read,
	&dsh_set,
	&dsh_reset,
	&dsh_check,
	&dsh_getpath,
	&dsh_help,
	&dsh_exit
};

int dsh_num_cmds(void) {
	return sizeof(dsh_cmd_str) / sizeof(char *);
}

int dsh_execute(char **args)
{
	int i;

	if (args[0] == NULL)
		return 0;

	for (i = 0; i < dsh_num_cmds(); i++)
		if (!strcmp(args[0], dsh_cmd_str[i]))
			return (*dsh_cmd_func[i])(args);

	fprintf(stderr, "Error: unrecognized command\n");
	return 0;
}

char **dsh_split_line(char *line)
{
	int bufsz = DSH_TOK_BUFSZ, pos = 0;
	char **tokens = malloc(bufsz * sizeof(char *));
	char *token;

	if (!tokens) {
		fprintf(stderr, "Error: allocation failed\n");
		return NULL;
	}

	token = strtok(line, DSH_TOK_DELIM);
	while (token != NULL) {
		tokens[pos] = token;
		pos++;

		if (pos >= bufsz) {
			bufsz += DSH_TOK_BUFSZ;
			tokens = realloc(tokens, bufsz * sizeof(char *));
			if (!tokens) {
				fprintf(stderr, "Error: allocation failed\n");
				return NULL;
			}
		}

		token = strtok(NULL, DSH_TOK_DELIM);
	}

	tokens[pos] = NULL;
	return tokens;
}

char *dsh_read_line(void)
{
	char *line = NULL;
	size_t bufsz = 0;

	if (getline(&line, &bufsz, stdin) == -1) {
		fprintf(stderr, "Error: failed to get line\n");
		return NULL;
	}
	return line;
}

int dsh_loop(void)
{
	int status;
	char *line;
	char **args;

	do {
		fprintf(stdout, "\x1B[37m" "duet: " "\x1B[0m");
		line = dsh_read_line();
		if (!line)
			return 1;

		args = dsh_split_line(line);
		if (!args) {
			free(line);
			return 1;
		}

		status = dsh_execute(args);

		free(line);
		free(args);
	} while (!status);

	return 0;
}

int cmd_debug(int argc, char **argv)
{
	fdnum = 0;

	fprintf(stdout, "Duet debugging shell\n"
			"====================\n\n"
			"Type 'help' for a list of available commands.\n\n");

	return dsh_loop();
}
