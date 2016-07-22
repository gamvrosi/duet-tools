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
#ifndef _DUET_SYSCALL_H
#define _DUET_SYSCALL_H

#include <asm/types.h>
#include <linux/limits.h>
#include "duet.h"

/* Status syscall args */
enum duet_status_flags {
	DUET_START = 1,
	DUET_STOP,
	DUET_REPORT,
	DUET_PRINT_BMAP,
	DUET_PRINT_ITEM,
	DUET_PRINT_LIST,
};

struct duet_task_attrs {
	__u8	id;
	int 	fd;
	char 	name[NAME_MAX];
	__u32	regmask;
	char	path[PATH_MAX];
};

struct duet_status_args {
	__u32 size;						/* in */
	union {
		/* DUET_START args */
		struct {
			__u16 maxtasks;				/* in */
		};

		/* DUET_PRINT_BIT, DUET_PRINT_ITEM args */
		struct {
			__u8 id;				/* in */
		};

		/* DUET_PRINT_LIST args */
		struct {
			__u16 numtasks;				/* out */
			struct duet_task_attrs tasks[0];	/* out */
		};
	};
};

#endif /* _DUET_SYSCALL_H */
