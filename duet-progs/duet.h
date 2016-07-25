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

/* TODO: Clean up */

#ifndef _DUET_H
#define _DUET_H

#include <asm/types.h>
#include <stddef.h>

#define MAX_TASKS	128

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({				\
		const typeof( ((type *)0)->member  ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member)  ); })

//#define DUET_DEBUG
#ifdef DUET_DEBUG
#define duet_dbg(...) fprintf(stdout, __VA_ARGS__)
#else
#define duet_dbg(...)
#endif /* DUET_DEBUG */

/*
 * Duet can be state-based, and/or event-based.
 *
 * Event-based Duet monitors events that occurred on a page, during its
 * time in the page cache: ADDED, REMOVED, DIRTY, and FLUSHED.
 *
 * State-based Duet monitors changes in the page cache since the last time
 * a notification was sent to the interested application. Registering for
 * EXIST informs the application of page additions or removals from the cache
 * (i.e. ADDED and REMOVED events cancel each other out if the application
 * hasn't been told in the meantime). Registering for MODIFIED events is a
 * similar model, where unreported DIRTY and FLUSHED events cancel each other.
 */
#define DUET_PAGE_ADDED		0x0001
#define DUET_PAGE_REMOVED	0x0002
#define DUET_PAGE_DIRTY		0x0004
#define DUET_PAGE_FLUSHED	0x0008
#define DUET_PAGE_MODIFIED	0x0010
#define DUET_PAGE_EXISTS	0x0020
#define DUET_FD_NONBLOCK	0x0040

/*
 * Item struct returned for processing.
 * The UUID currently consists of the inode number, the inode generation
 * (to help us identify cases of inode reuse), and the task id.
 * For state-based duet, we mark a page if it EXISTS or is MODIFIED.
 * For event-based duet, we mark a page added, removed, dirtied, and/or flushed.
 * Acceptable event combinations will differ based on the task's subscription.
 */
struct duet_uuid {
	unsigned long	ino;
	__u32		gen;
	__u8		tid;
};

struct duet_item {
	struct duet_uuid	uuid;
	unsigned long		idx;
	__u16			state;
};

int duet_register(const char *name, __u32 regmask, const char *path);
int duet_deregister(int duet_fd, int tid);
int duet_fetch(int duet_fd, int tid, struct duet_item *items, int *count);
int duet_set_done(struct duet_uuid uuid);
int duet_reset_done(struct duet_uuid uuid);
int duet_check_done(struct duet_uuid uuid);
char *duet_get_path(struct duet_uuid uuid);
int duet_print_bmap(int id);
int duet_print_item(int id);
int duet_print_list(int numtasks);

#endif /* _DUET_H */
