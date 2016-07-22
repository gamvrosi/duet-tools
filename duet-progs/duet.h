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
 * Duet can be either state- and/or event-based.
 * Event-based Duet monitors events that have happened on a page, which include
 * all events in the lifetime of a cache page: ADDED, REMOVED, DIRTY, FLUSHED.
 * Add and remove events are triggered when a page __descriptor__ is inserted or
 * removed from the page cache. Modification events are triggered when the page
 * is dirtied (nb: during writes, pages are added, then dirtied), and flush
 * events are triggered when a page is marked for writeback.
 * State-based Duet monitors changes in the page cache. Registering for EXISTS
 * events means that fetch will be returning ADDED or REMOVED events if the
 * state of the page changes since the last fetch (i.e. the two events cancel
 * each other out). Registering for MODIFIED events means that fetch will be
 * returning DIRTY or FLUSHED events if the state of the page changes since the
 * last fetch.
 */
#define DUET_PAGE_ADDED		0x0001
#define DUET_PAGE_REMOVED	0x0002
#define DUET_PAGE_DIRTY		0x0004
#define DUET_PAGE_FLUSHED	0x0008
#define DUET_PAGE_MODIFIED	0x0010
#define DUET_PAGE_EXISTS	0x0020

#define DUET_UUID_INO(uuid)	((unsigned long)(uuid & 0xffffffff))
#define DUET_UUID_GEN(uuid)	((unsigned long)(uuid >> 32))

/*
 * Item struct returned for processing. For both state- and event- based duet,
 * we return 4 bits, for page addition, removal, dirtying, and flushing. The
 * acceptable combinations, however, will differ based on what the task has
 * subscribed for.
 */
struct duet_item {
	unsigned long long	uuid;
	unsigned long		idx;
	__u16			state;
};

int open_duet_dev(void);
void close_duet_dev(int duet_fd);

int duet_register(const char *name, __u32 regmask, const char *path);
int duet_deregister(int duet_fd, int tid);
int duet_fetch(int duet_fd, int tid, struct duet_item *items, int *count);
int duet_check_done(int duet_fd, int tid, __u64 idx, __u32 count);
int duet_set_done(int duet_fd, int tid, __u64 idx, __u32 count);
int duet_unset_done(int duet_fd, int tid, __u64 idx, __u32 count);
int duet_get_path(int duet_fd, int tid, unsigned long long uuid, char *path);
int duet_print_bmap(int id);
int duet_print_item(int id);
int duet_print_list(int numtasks);

#endif /* _DUET_H */
