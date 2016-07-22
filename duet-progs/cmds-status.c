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

static const char * const status_cmd_group_usage[] = {
	"duet status <command>",
	NULL
};

static const char * const cmd_status_start_usage[] = {
	"duet status start [-n tasks]",
	"Enable the Duet framework.",
	"Initializes and enables the duet framework. Only tasks registered",
	"after running this command will be monitored by the framework.",
	"Ensure the framework is off, otherwise this command will fail.",
	"",
	"-n	max number of concurrently running tasks (default: 8)",
	NULL
};

static const char * const cmd_status_stop_usage[] = {
	"duet status stop",
	"Disable the Duet framework.",
	"Terminates and cleans up any metadata kept by the duet framework.",
	"Any tasks running will no longer be monitored by the framework,",
	"but will continue to function. Ensure the framework is on,",
	"otherwise this command will fail.",
	NULL
};

static const char * const cmd_status_report_usage[] = {
	"duet status report",
	"Report on the status of the Duet framework.",
	"Reports whether the Duet framework is online or offline.",
	NULL
};

static int cmd_status_start(int argc, char **argv)
{
	int c, ret = 0;
	struct duet_status_args args;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	optind = 1;
	while ((c = getopt(argc, argv, "n:")) != -1) {
		switch (c) {
		case 'n':
			errno = 0;
			args.maxtasks = (__u8)strtoul(optarg, NULL, 10);
			if (errno) {
				perror("strtoul: invalid number of tasks");
				usage(cmd_status_start_usage);
			}
			break;
		default:
			fprintf(stderr, "Unknown option %c\n", (char)c);
			usage(cmd_status_start_usage);
		}
	}

	if (argc != optind)
		usage(cmd_status_start_usage);

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_START, &args);
	if (ret < 0) {
		perror("duet status: start failed");
		usage(cmd_status_start_usage);
	}

	return ret;
}

static int cmd_status_stop(int argc, char **argv)
{
	int ret = 0;
	struct duet_status_args args;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_STOP, &args);
	if (ret < 0) {
		perror("duet status: stop failed");
		usage(cmd_status_stop_usage);
	}

	return ret;
}

static int cmd_status_report(int argc, char **argv)
{
	int ret = 0;
	struct duet_status_args args;

	memset(&args, 0, sizeof(args));
	args.size = sizeof(args);

	/* Call syscall x86_64 #329: duet_status */
	ret = syscall(329, DUET_REPORT, &args);
	if (ret < 0) {
		perror("duet status: report failed");
		usage(cmd_status_report_usage);
		return ret;
	}

	if (!ret)
		fprintf(stdout, "Duet framework status: Offline.\n");
	else
		fprintf(stdout, "Duet framework status: Online.\n");

	return 0;
}

const struct cmd_group status_cmd_group = {
	status_cmd_group_usage, NULL, {
		{ "start", cmd_status_start, cmd_status_start_usage, NULL, 0 },
		{ "stop", cmd_status_stop, cmd_status_stop_usage, NULL, 0 },
		{ "report", cmd_status_report, cmd_status_report_usage, NULL, 0 },
	}
};

int cmd_status(int argc, char **argv)
{
	return handle_command_group(&status_cmd_group, argc, argv);
}
