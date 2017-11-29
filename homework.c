/*
 * file:        homework.c
 * description: skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 * $Id: homework.c 410 2011-11-07 18:42:45Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "blkdev.h"

/********** MIRRORING ***************/

/* example state for mirror device. See mirror_create for how to
 * initialize a struct blkdev with this.
 */
struct mirror_dev {
    struct blkdev *disks[2];    /* flag bad disk by setting to NULL */
    int nblks;
};

// Amanda: Return the number of blocks nblks
static int mirror_num_blocks(struct blkdev *dev)
{
    /* your code here */
    struct mirror_dev *mdev = dev->private;
    return mdev->nblks;
}

/* read from one of the sides of the mirror. (if one side has failed,
 * it had better be the other one...) If both sides have failed,
 * return an error.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should close the
 * device and flag it (e.g. as a null pointer) so you won't try to use
 * it again. 
 */
static int mirror_read(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    struct mirror_dev *mdev = dev->private;

    //Amanda: check for bad address errors
    if (first_blk<0 || first_blk+num_blks>mdev->nblks)
        return E_BADADDR;
    
    int i;
    int disk_content[2];
    for (i = 0; i<2; i++){
        struct blkdev *disk = mdev->disks[i];
        if (disk == NULL){
            disk_content[i] = E_UNAVAIL;
            continue;
        }
        else{
            disk_content[i] = disk -> ops -> read ( disk, first_blk,num_blks, buf );

            if (disk_content[i] == E_UNAVAIL){
                disk->ops->close(disk);
                mdev->disks[i] = NULL;
            }
        }

        if (disk_content[0] == SUCCESS || disk_content[1]==SUCCESS){
            return SUCCESS;
        } 
        else
            return E_UNAVAIL;
    }
}


/* write to both sides of the mirror, or the remaining side if one has
 * failed. If both sides have failed, return an error.
 * Note that a write operation may indicate that the underlying device
 * has failed, in which case you should close the device and flag it
 * (e.g. as a null pointer) so you won't try to use it again.
 */
static int mirror_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf)
{
    /* your code here */
    struct mirror_dev *mdev = dev->private;

    //Amanda: check for bad address errors
    if (first_blk<0 || first_blk+num_blks>mdev->nblks)
        return E_BADADDR;

    int i;
    int disk_content[2];
    //Amanda: disk_content holds the contents read by the read function or E_UNAVAIL if the disk fails

    for (i = 0; i<2; i++){
        struct blkdev *disk = mdev->disks[i];
        if (disk == NULL){
            // if disk fails, close on the corresponding blkdev 
            disk_content[i] = E_UNAVAIL;
            continue;
        }
        else{
            disk_content[i] = disk -> ops -> write ( disk, first_blk,num_blks, buf );

            if (disk_content[i] == E_UNAVAIL){
                disk->ops->close(disk);
                disk = NULL;
            }
        }

        if (disk_content[0] == SUCCESS || disk_content[1]==SUCCESS){
            return SUCCESS;
        } 
        else
            return E_UNAVAIL;
    }
}

/* clean up, including: close any open (i.e. non-failed) devices, and
 * free any data structures you allocated in mirror_create.
 */
static void mirror_close(struct blkdev *dev)
{
    /* your code here */
    struct mirror_dev *mdev = dev->private;

    //Amanda: closing non-failed devices
    int i;
    for (i = 0; i<2; i++){
        struct blkdev *disk = mdev->disks[i];
        if (disk != NULL)
            disk->ops->close(disk);
    }

    //Amanda: freeing mdev and dev allocated in mirror_create
    free(mdev);
    free(dev);

}

struct blkdev_ops mirror_ops = {
    .num_blocks = mirror_num_blocks,
    .read = mirror_read,
    .write = mirror_write,
    .close = mirror_close
};

/* create a mirrored volume from two disks. Do not write to the disks
 * in this function - you should assume that they contain identical
 * contents. 
 */
struct blkdev *mirror_create(struct blkdev *disks[2])
{
    struct blkdev *dev = malloc(sizeof(*dev));
    struct mirror_dev *mdev = malloc(sizeof(*mdev));

    /* your code here */
    // Amanda: check if dev and mdev are not NULL
    assert(dev != NULL || mdev != NULL);

    //Amanda: if the size of the two disks are not of the same size, print an error and return NULL
    if (disks[0]->ops->num_blocks(disks[0]) != disks[1]->ops->num_blocks(disks[1])){
        printf("Error: The two disk sizes are not the same.\n");
        return NULL;
    }

    // Amanda: copying two disks to the correspoding disks in mdev
     mdev->disks[0] = disks[0];
     mdev->disks[1] = disks[1];
     // Amanda: also copying the number of blocks to mirror dev (assuming size of both disks are the same)
     mdev->nblks = disks[0]->ops->num_blocks(disks[0]);

    dev->private = mdev;
    dev->ops = &mirror_ops;

    return dev;
}

/* replace failed device 'i' (0 or 1) in a mirror. Note that we assume
 * the upper layer knows which device failed. You will need to
 * replicate content from the other underlying device before returning
 * from this call.
 */
int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    struct mirror_dev *mdev = volume->private;

    if (newdisk->ops->num_blocks(newdisk) != mdev->nblks)
        return E_SIZE;

    // Amanda: Replicate other non-failed disk
    // Amanda: If failed disk is disks[0] set working disk as disks[1], vice versa
    struct blkdev *working_disk = mdev -> disks[1];

    if (i == 1){
        working_disk = mdev -> disks[0];
    }
    else if (i == 0){
        working_disk = mdev -> disks[1];
    }

    char buffer[ BLOCK_SIZE ];
    int LBA;
    for ( LBA = 0 ; LBA < mdev -> nblks ; LBA ++ ) {
        int val = working_disk -> ops -> read( working_disk, LBA, 1, buffer );
        if ( val != SUCCESS ) {
            return val;
        }
        val = newdisk -> ops -> write ( newdisk, LBA, 1, buffer );
        if ( val != SUCCESS ) {
            return val;
        }
    }

    // Replace failed disk with new disk 
    mdev->disks[i] = newhdisk;
    return SUCCESS;

}

/**********  RAID0 ***************/

int raid0_num_blocks(struct blkdev *dev)
{
    return 0;
}

/* read blocks from a striped volume. 
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should (a) close the
 * device and (b) return an error on this and all subsequent read or
 * write operations. 
 */
static int raid0_read(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    return 0;
}

/* write blocks to a striped volume.
 * Again if an underlying device fails you should close it and return
 * an error for this and all subsequent read or write operations.
 */
static int raid0_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf)
{
    return 0;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in stripe_create. 
 */
static void raid0_close(struct blkdev *dev)
{
}

/* create a striped volume across N disks, with a stripe size of
 * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
 * 4..7 on disks[1], etc.)
 * Check the size of the disks to compute the final volume size, and
 * fail (return NULL) if they aren't all the same.
 * Do not write to the disks in this function.
 */
struct blkdev *raid0_create(int N, struct blkdev *disks[], int unit)
{
    return NULL;
}

/**********   RAID 4  ***************/

/* helper function - compute parity function across two blocks of
 * 'len' bytes and put it in a third block. Note that 'dst' can be the
 * same as either 'src1' or 'src2', so to compute parity across N
 * blocks you can do: 
 *
 *     void **block[i] - array of pointers to blocks
 *     dst = <zeros[len]>
 *     for (i = 0; i < N; i++)
 *        parity(block[i], dst, dst);
 *
 * Yes, it could be faster. Don't worry about it.
 */
void parity(int len, void *src1, void *src2, void *dst)
{
    unsigned char *s1 = src1, *s2 = src2, *d = dst;
    int i;
    for (i = 0; i < len; i++)
        d[i] = s1[i] ^ s2[i];
}

/* read blocks from a RAID 4 volume.
 * If the volume is in a degraded state you may need to reconstruct
 * data from the other stripes of the stripe set plus parity.
 * If a drive fails during a read and all other drives are
 * operational, close that drive and continue in degraded state.
 * If a drive fails and the volume is already in a degraded state,
 * close the drive and return an error.
 */
static int raid4_read(struct blkdev * dev, int first_blk,
                      int num_blks, void *buf) 
{
    return 0;
}

/* write blocks to a RAID 4 volume.
 * Note that you must handle short writes - i.e. less than a full
 * stripe set. You may either use the optimized algorithm (for N>3
 * read old data, parity, write new data, new parity) or you can read
 * the entire stripe set, modify it, and re-write it. Your code will
 * be graded on correctness, not speed.
 * If an underlying device fails you should close it and complete the
 * write in the degraded state. If a drive fails in the degraded
 * state, close it and return an error.
 * In the degraded state perform all writes to non-failed drives, and
 * forget about the failed one. (parity will handle it)
 */
static int raid4_write(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    return 0;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in raid4_create. 
 */
static void raid4_close(struct blkdev *dev)
{
}

/* Initialize a RAID 4 volume with strip size 'unit', using
 * disks[N-1] as the parity drive. Do not write to the disks - assume
 * that they are properly initialized with correct parity. (warning -
 * some of the grading scripts may fail if you modify data on the
 * drives in this function)
 */
struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)
{
    return NULL;
}

/* replace failed device 'i' in a RAID 4. Note that we assume
 * the upper layer knows which device failed. You will need to
 * reconstruct content from data and parity before returning
 * from this call.
 */
int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    return SUCCESS;
}

