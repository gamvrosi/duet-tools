/*
 * Copyright (C) 2016 George Amvrosiadis.  All rights reserved.
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
#ifndef _DUET_IOCTL_H
#define _DUET_IOCTL_H

#include <asm/types.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include "duet.h"

#define DUET_IOC_MAGIC	0xDE

/* Status ioctl flags */
#define DUET_STATUS_START	0x0001
#define DUET_STATUS_STOP	0x0002
#define DUET_STATUS_REPORT	0x0004
#define DUET_STATUS_PRINT_BMAP	0x0008
#define DUET_STATUS_PRINT_ITEM	0x0010
#define DUET_STATUS_PRINT_LIST	0x0020

struct duet_task_attrs {
	__u8 	id;
	int	fd;
	char 	name[NAME_MAX];
	__u32	regmask;
	char	path[PATH_MAX];
};

struct duet_ioctl_status_args {
	__u32 size;						/* in */
	__u16 flags;						/* in */
	union {
		/* START args */
		struct {
			__u16 maxtasks;				/* in */
		};

		/* PRINT_BIT, PRINT_ITEM args */
		struct {
			__u8 id;				/* in */
		};

		/* PRINT_LIST args */
		struct {
			__u16 numtasks;				/* out */
			struct duet_task_attrs tasks[0];	/* out */
		};
	};
};

struct duet_ioctl_init_args {
	__u32	size;						/* in */
	char	name[NAME_MAX];					/* in */
	__u32	regmask;					/* in */
	char	path[PATH_MAX];					/* in */
};

/* Bitmap ioctl flags */
#define DUET_BMAP_SET		0x0001
#define DUET_BMAP_RESET		0x0002
#define DUET_BMAP_CHECK		0x0004

struct duet_ioctl_bmap_args {
	__u32	size;						/* in */
	__u16	flags;						/* in */
	struct	duet_uuid uuid;					/* in */
};

struct duet_ioctl_gpath_args {
	__u32 size;						/* in */
	struct duet_uuid uuid;					/* in */
	char path[PATH_MAX];					/* out */
};

#define DUET_IOC_STATUS	_IOWR(DUET_IOC_MAGIC, 1, struct duet_ioctl_status_args)
#define DUET_IOC_INIT	_IOWR(DUET_IOC_MAGIC, 2, struct duet_ioctl_init_args)
#define DUET_IOC_BMAP	_IOWR(DUET_IOC_MAGIC, 3, struct duet_ioctl_bmap_args)
#define DUET_IOC_GPATH	_IOWR(DUET_IOC_MAGIC, 4, struct duet_ioctl_gpath_args)

#endif /* _DUET_IOCTL_H */
