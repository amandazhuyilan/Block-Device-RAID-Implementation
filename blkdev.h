/*
 * file:        blkdev.h
 * description: Block device structure for CS 5600 HW2
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 * $Id: blkdev.h 413 2011-11-09 04:18:56Z pjd $
 */
#ifndef __BLKDEV_H__
#define __BLKDEV_H__

#define BLOCK_SIZE 512   /* 512-byte unit for all blkdev addressing in HW3 */

struct blkdev {
    struct blkdev_ops *ops;
    void *private;
};

struct blkdev_ops {
    int  (*num_blocks)(struct blkdev *dev);
    int  (*read)(struct blkdev * dev, int first_blk, int num_blks, void *buf);
    int  (*write)(struct blkdev * dev, int first_blk, int num_blks, void *buf);
    void (*close)(struct blkdev *dev);
};

enum {SUCCESS = 0, E_BADADDR = -1, E_UNAVAIL = -2, E_SIZE = -3};

extern struct blkdev *image_create(char *path);
extern void image_fail(struct blkdev *);
extern struct blkdev *mirror_create(struct blkdev **);
extern int mirror_replace(struct blkdev *, int, struct blkdev *);
extern struct blkdev *raid0_create(int, struct blkdev **, int);
extern struct blkdev *raid4_create(int, struct blkdev **, int);
extern int raid4_replace(struct blkdev *, int, struct blkdev *);

#endif
