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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <linux/limits.h>
#include <duet/duet.h>

#define FETCH_ITEMS	512

#define PRINT_STATE(mask) \
	fprintf(stdout, "  Events: %s%s%s%s\n", \
		((mask & DUET_PAGE_ADDED) ? "ADDED " : ""), \
		((mask & DUET_PAGE_REMOVED) ? "REMOVED " : ""), \
		((mask & DUET_PAGE_DIRTY) ? "DIRTY " : ""), \
		((mask & DUET_PAGE_FLUSHED) ? "FLUSHED " : ""));

static volatile int got_sigint = 0;

void handle_sigint(int signal)
{
	if (signal != SIGINT)
		return;

	got_sigint = 1;
}

void usage(int err)
{
	fprintf(stderr,
		"\n"
		"dummy is a program meant to demonstrate how to use the Duet\n"
		"framework.\n"
		"\n"
		"Usage: dummy [OPTION]...\n"
		"\n"
		"Program Options\n"
		" -f <freq>     event fetching frequency in msec (def: 10ms)\n"
		" -d <dur>      program execution time in sec\n"
		" -o            use Duet (if not set, Duet Options are ignored)\n"
		" -h            print this usage information\n"
		"\n"
		"Duet Options\n"
		" -e            register for event-based Duet (def: state-based)\n"
		" -p <path>     directory to register with Duet (def: '/')\n"
		" -g            get file path for every event received\n"
		"\n");

		exit(err);
}

int main(int argc, char *argv[])
{
	int freq = 10;
	int duration = 0;
	int use_duet = 0;
	int evtbased = 0;
	int getpath = 0;
	int keep_running = 0;
	int c, dfd = 0, ret = 0;
	long total_items = 0;
	long total_fetches = 0;
	__u32 regmask;
	char path[PATH_MAX] = "/";
	size_t bufsize;
	char *buf, *pos, *gpath;
	struct duet_item *itm;
	struct timespec slp = {0, 0};

	signal(SIGINT, handle_sigint);

	while ((c = getopt(argc, argv, "f:d:ohep:g")) != -1) {
		switch (c) {
		case 'f': /* Fetching frequency, in mseconds */
			freq = atoi(optarg);
			if (freq <= 0) {
				fprintf(stderr, "Error: invalid fetch frequency\n");
				usage(1);
			}
			break;

		case 'd': /* Program execution duration, in seconds */
			duration = atoi(optarg);
			if (duration < 0) {
				fprintf(stderr, "Error: invalid duration\n");
				usage(1);
			}
			break;

		case 'o': /* Use Duet */
			use_duet = 1;
			break;

		case 'h': /* Display usage info */
			usage(0);
			break;

		case 'e': /* Register for event-based Duet */
			evtbased = 1;
			break;

		case 'p': /* Specify directory to register with Duet */
			if (strnlen(optarg, PATH_MAX + 1) > PATH_MAX) {
				fprintf(stderr, "Error: specified path too long\n");
				usage(1);
			}
			strncpy(path, optarg, PATH_MAX);
			break;

		case 'g': /* Get file path for every event */
			getpath = 1;
			break;
		default:
			fprintf(stderr, "Unknown argument!\n");
			usage(1);
		}
	}

	bufsize = FETCH_ITEMS * sizeof(struct duet_item);
	buf = malloc(bufsize);
	if (!buf) {
		fprintf(stderr, "read: buffer allocation failed\n");
		return 1;
	}

	if (!duration) {
		fprintf(stdout, "Warning: Dummy will run until Ctrl-C is pressed.\n");
		keep_running = 1;
	}

	printf("Running dummy for %d sec. Fetching every %d ms.\n",
		duration, freq);

	/* Convert duration to mseconds and set nanosleep time */
	duration *= 1000;
	slp.tv_nsec = (freq * (long) 1E6) % (long) 1E9;
	slp.tv_sec = (freq * (long) 1E6) / (long) 1E9;

	if (evtbased)
		regmask = DUET_PAGE_ADDED | DUET_FD_NONBLOCK;
	else
		regmask = DUET_PAGE_EXISTS | DUET_FD_NONBLOCK;

	/* Register with Duet framework */
	if (use_duet) {
		dfd = duet_register("dummy", regmask, path);
		if (dfd <= 0) {
			fprintf(stderr, "Error: Duet registration failed\n");
			ret = 1;
			goto done_close;
		}
	}

	/* Use specified fetching frequency */
	while (duration > 0 || keep_running) {
		if (!use_duet)
			goto read_done;

		ret = read(dfd, buf, bufsize);
		if (!ret && errno != EAGAIN) {
			fprintf(stderr, "Error: Duet fetch failed\n");
			ret = 1;
			goto done_dereg;
		}

		if (!getpath)
			goto count;

		fprintf(stdout, "Printing received items and paths:\n\n");
		for (pos = buf; pos < buf+ret; pos += sizeof(*itm)) {
			itm = (struct duet_item *)pos;

			gpath = duet_get_path(itm->uuid);

			fprintf(stdout, "  Inode #%lu\n", itm->uuid.ino);
			fprintf(stdout, "  Offset: %llub\n",
					(__u64)itm->idx << 12);
			PRINT_STATE(itm->state);
			if (gpath)
				fprintf(stdout, "  Path: %s\n\n", gpath);
		}
count:
		total_items += ret / sizeof(struct duet_item);
		total_fetches++;

read_done:
		if (nanosleep(&slp, NULL) < 0) {
			fprintf(stderr, "Error: nanosleep failed\n");
			ret = 1;
			goto done_dereg;
		}

		if (!keep_running) {
			if (duration % 1000 == 0)
				fprintf(stdout, "Time left: %dsec\n", duration / 1000);
			duration -= freq;
		}

		if (keep_running && got_sigint)
			keep_running = 0;
	}

done_dereg:
	if (use_duet)
		close(dfd);

done_close:
	if (use_duet)
		fprintf(stdout, "Fetched %ld events, or %lf events/ms\n",
			total_items, ((double) total_items)/total_fetches);

	return ret;
}
