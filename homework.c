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
#include <string.h>
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
static int mirror_num_blocks(struct blkdev *dev) {
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
                       int num_blks, void *buf) {
    struct mirror_dev *mdev = dev->private;

    //Amanda: check for bad address errors
    if (first_blk < 0 || first_blk + num_blks > mdev->nblks)
        return E_BADADDR;

    int i;
    int disk_content[2];
    for (i = 0; i < 2; i++) {
        struct blkdev *disk = mdev->disks[i];
        if (disk == NULL) {
            disk_content[i] = E_UNAVAIL;
            continue;
        } else {
            disk_content[i] = disk -> ops -> read ( disk, first_blk, num_blks, buf );

            if (disk_content[i] == E_UNAVAIL) {
                //disk->ops->close(disk);
                mdev->disks[i] = NULL;
            }
        }

        if (disk_content[0] == SUCCESS || disk_content[1] == SUCCESS) {
            return SUCCESS;
        } else
            return E_UNAVAIL;
    }

    return SUCCESS;
}


/* write to both sides of the mirror, or the remaining side if one has
 * failed. If both sides have failed, return an error.
 * Note that a write operation may indicate that the underlying device
 * has failed, in which case you should close the device and flag it
 * (e.g. as a null pointer) so you won't try to use it again.
 */
static int mirror_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf) {
    /* your code here */
    struct mirror_dev *mdev = dev->private;

    //Amanda: check for bad address errors
    if (first_blk < 0 || first_blk + num_blks > mdev->nblks)
        return E_BADADDR;

    int i;
    int disk_content[2];
    //Amanda: disk_content holds the contents read by the read function or E_UNAVAIL if the disk fails

    for (i = 0; i < 2; i++) {
        struct blkdev *disk = mdev->disks[i];
        if (disk == NULL) {
            // if disk fails, close on the corresponding blkdev
            disk_content[i] = E_UNAVAIL;
            continue;
        } else {
            disk_content[i] = disk -> ops -> write ( disk, first_blk, num_blks, buf );

            if (disk_content[i] == E_UNAVAIL) {
                //disk->ops->close(disk);
                disk = NULL;
            }
        }

        if (disk_content[0] == SUCCESS || disk_content[1] == SUCCESS) {
            return SUCCESS;
        } else
            return E_UNAVAIL;
    }

    return SUCCESS;
}

/* clean up, including: close any open (i.e. non-failed) devices, and
 * free any data structures you allocated in mirror_create.
 */
static void mirror_close(struct blkdev *dev) {
    /* your code here */
    struct mirror_dev *mdev = dev->private;

    //Amanda: closing non-failed devices
    int i;
    for (i = 0; i < 2; i++) {
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
struct blkdev *mirror_create(struct blkdev *disks[2]) {
    struct blkdev *dev = malloc(sizeof(*dev));
    struct mirror_dev *mdev = malloc(sizeof(*mdev));

    /* your code here */
    // Amanda: check if dev and mdev are not NULL
    assert(dev != NULL || mdev != NULL);

    //Amanda: if the size of the two disks are not of the same size, print an error and return NULL
    if (disks[0]->ops->num_blocks(disks[0]) != disks[1]->ops->num_blocks(disks[1])) {
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
int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk) {
    struct mirror_dev *mdev = volume->private;

    if (newdisk->ops->num_blocks(newdisk) != mdev->nblks)
        return E_SIZE;

    // Amanda: Replicate other non-failed disk
    // Amanda: If failed disk is disks[0] set working disk as disks[1], vice versa
    struct blkdev *working_disk = mdev -> disks[1];

    if (i == 1) {
        working_disk = mdev -> disks[0];
    } else if (i == 0) {
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
    mdev->disks[i] = newdisk;
    return SUCCESS;
}

/**********  RAID0 ***************/

/* Custom struct for raid0 devices similar to
 * block dev and image dev
 */
struct raid0_dev {
    struct blkdev **disks;
    int disksCount;
    int unitSize;
    int usableBlocksPerDisk;
};

int raid0_num_blocks(struct blkdev *dev) {
    struct raid0_dev *r0dev = dev->private;
    return r0dev->usableBlocksPerDisk * r0dev->disksCount;
}


/* Combines raid0's read and write logic */
static int raid0_read_write(struct blkdev * dev, int first_blk,
                            int num_blks, void *buf, int isWrite) {
    struct raid0_dev *r0dev = dev->private;

    //check for bad address errors
    if (first_blk < 0 || first_blk + num_blks > dev->ops->num_blocks(dev))
        return E_BADADDR;

    int unitSize = r0dev->unitSize;
    int disksCount = r0dev->disksCount;
    int stripeSize = unitSize * disksCount;
    int i;
    for (i = num_blks; i > 0;) {
        int blockInDisk = (first_blk % unitSize) + (first_blk / stripeSize) * unitSize;
        int blocksRemaining = unitSize - (blockInDisk % unitSize);
        int diskNum = (first_blk % stripeSize) / unitSize ;

        int len = (i <= blocksRemaining) ? i : blocksRemaining;
        struct blkdev *diskToUse = r0dev->disks[diskNum];
        int returnValue;
        if (isWrite)
            returnValue = diskToUse->ops->write(diskToUse, blockInDisk, len, buf);
        else
            returnValue = diskToUse->ops->read(diskToUse, blockInDisk, len, buf);

        if (returnValue == E_UNAVAIL) {
            diskToUse->ops->close(diskToUse);
            return returnValue;
        } else if (returnValue == SUCCESS) {
            buf = buf + len * BLOCK_SIZE;
            i = i - len;
            first_blk = first_blk + len;
        } else return returnValue;
    }
    return SUCCESS;
}


/* read blocks from a striped volume.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should (a) close the
 * device and (b) return an error on this and all subsequent read or
 * write operations.
 */
static int raid0_read(struct blkdev * dev, int first_blk,
                      int num_blks, void *buf) {
    return raid0_read_write(dev, first_blk, num_blks, buf, 0);
}

/* write blocks to a striped volume.
 * Again if an underlying device fails you should close it and return
 * an error for this and all subsequent read or write operations.
 */
static int raid0_write(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf) {
    return raid0_read_write(dev, first_blk, num_blks, buf, 1);
}

/* clean up, including: close all devices and free any data structures
 * you allocated in stripe_create.
 */
static void raid0_close(struct blkdev *dev) {
    struct raid0_dev *r0dev = dev->private;
    int i;
    for (i = 0; i < r0dev->disksCount; i++) {
        struct blkdev *disk = r0dev->disks[i];
        disk->ops->close(disk);
    }
    free(r0dev);
    free(dev);
}


/* Overriding the basic operations of a block device for raid0 */
struct blkdev_ops raid0_ops = {
    .num_blocks = raid0_num_blocks,
    .read = raid0_read,
    .write = raid0_write,
    .close = raid0_close
};


/* helper function - checks if the given disks are of same size
 * returns 1 on success, -1 otherwise.
 */
int checkDisksSameSize(int N, struct blkdev *disks[]) {
    if (N < 1) {
        printf("Invalid disk count.\n");
        return -1;
    }
    int i;
    int num_blocks = disks[0]->ops->num_blocks(disks[0]);
    for (i = 1; i < N; i++)
        if (disks[i]->ops->num_blocks(disks[i]) != num_blocks) {
            printf("Given disks are not of same size.\n");
            return -1;
        }
    return 1;
}


/* create a striped volume across N disks, with a stripe size of
 * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
 * 4..7 on disks[1], etc.)
 * Check the size of the disks to compute the final volume size, and
 * fail (return NULL) if they aren't all the same.
 * Do not write to the disks in this function.
 */
struct blkdev *raid0_create(int N, struct blkdev *disks[], int unit) {
    if (checkDisksSameSize(N, disks) == -1)
        return NULL;

    int num_blocks = disks[0]->ops->num_blocks(disks[0]);

    // Saran: Allocate memory after initial check
    struct blkdev *dev = malloc(sizeof(*dev));
    struct raid0_dev *r0dev = malloc(sizeof(*r0dev));

    // Saran: Assigning references to the blkdev interface
    r0dev->disks = disks;
    r0dev->disksCount = N;
    r0dev->unitSize = unit;
    r0dev->usableBlocksPerDisk = (num_blocks / unit) * unit;
    dev->private = r0dev;
    dev->ops = &raid0_ops;

    return dev;
}


/**********   RAID 4  ***************/

/* Custom struct for raid4 devices similar to
 * block dev and image dev
 */
struct raid4_dev {
    struct blkdev **disks;
    struct blkdev *parityDisk;
    int dataDisksCount;
    int unitSize;
    int usableBlocksPerDisk;
    int isDegraded;
    int failedDiskNumber;
};

int raid4_num_blocks(struct blkdev *dev) {
    struct raid4_dev *r4dev = dev->private;
    return r4dev->usableBlocksPerDisk * r4dev->dataDisksCount;
}

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
void parity(int len, void *src1, void *src2, void *dst) {
    unsigned char *s1 = src1, *s2 = src2, *d = dst;
    int i;
    for (i = 0; i < len; i++)
        d[i] = s1[i] ^ s2[i];
}


// INVARIENT: dev->private->failedDiskNumber != -1
void recoverFailedDiskData(struct blkdev *dev, int blockInDisk, int len, void *outputBuf) {
    struct raid4_dev *r4dev = dev->private;
    int failedDiskNumber = r4dev->failedDiskNumber;

    // dst = <zeros[len]>
    memset(outputBuf, '\0', len * BLOCK_SIZE);
    char tempBuf[len * BLOCK_SIZE];
    int i;
    for (i = 0; i < r4dev->dataDisksCount + 1; i++) {
        if (i != failedDiskNumber) {
            struct blkdev *disk = r4dev->disks[i];
            disk->ops->read(disk, blockInDisk, len, tempBuf);
            parity(len * BLOCK_SIZE, tempBuf, outputBuf, outputBuf);
        }
    }
}

/* Sets computed parity into parityBuf
 * Returns the disk num if it fails while computing parity.
 */
int computeParity(struct blkdev *dev, int blockInDisk, int len, void *dataForFailedDisk, void *parityBuf) {
    struct raid4_dev *r4dev = dev->private;
    int parityDiskNum = r4dev->dataDisksCount;
    int failedDiskNumber = r4dev->failedDiskNumber;

    // if parity disk is degraded
    if (r4dev->isDegraded && (r4dev->failedDiskNumber == parityDiskNum))
        return parityDiskNum;

    // dst = <zeros[len]>
    memset(parityBuf, '\0', len * BLOCK_SIZE);
    char tempBuf[len * BLOCK_SIZE];
    int i;
    for (i = 0; i < r4dev->dataDisksCount; i++) {
        if (failedDiskNumber == i) {
            memcpy(tempBuf, dataForFailedDisk, len * BLOCK_SIZE);
        } else {
            struct blkdev *disk = r4dev->disks[i];
            int result = disk->ops->read(disk, blockInDisk, len, tempBuf);
            if (result == E_UNAVAIL)
                return i;
        }
        parity(len * BLOCK_SIZE, tempBuf, parityBuf, parityBuf);
    }
    return -1;
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
                      int num_blks, void *buf) {
    struct raid4_dev *r4dev = dev->private;

    //check for bad address errors
    if (first_blk < 0 || first_blk + num_blks > dev->ops->num_blocks(dev))
        return E_BADADDR;

    int unitSize = r4dev->unitSize;
    int disksCount = r4dev->dataDisksCount;
    int stripeSize = unitSize * disksCount;
    int i;
    for (i = num_blks; i > 0;) {
        int blockInDisk = (first_blk % unitSize) + (first_blk / stripeSize) * unitSize;
        int blocksRemaining = unitSize - (blockInDisk % unitSize);
        int diskNum = (first_blk % stripeSize) / unitSize ;

        int len = (i <= blocksRemaining) ? i : blocksRemaining;
        struct blkdev *diskToUse = r4dev->disks[diskNum];
        int returnValue;

        if (diskToUse != NULL)
            returnValue = diskToUse->ops->read(diskToUse, blockInDisk, len, buf);
        else returnValue = E_UNAVAIL;

        if (returnValue == E_UNAVAIL) {
            // if the raid4 device is already degraded
            if (r4dev->isDegraded) {
                // and if the known failed disk is not this disk, return E_UNAVAIL
                if (r4dev->failedDiskNumber != diskNum)
                    return E_UNAVAIL;
            } else {
                // marking the raid4 device as degraded
                r4dev->isDegraded = 1;
                r4dev->failedDiskNumber = diskNum;
            }
            // and read into buffer the recovered data for the failed disk
            recoverFailedDiskData(dev, blockInDisk, len, buf);
        } else if (returnValue == SUCCESS) {
        } else return returnValue;

        // incrementing buffer, iter variable and the block for next iteration
        buf = buf + len * BLOCK_SIZE;
        i = i - len;
        first_blk = first_blk + len;
    }
    return SUCCESS;
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
                       int num_blks, void *buf) {
    struct raid4_dev *r4dev = dev->private;
    struct blkdev *parityDisk = r4dev->parityDisk;

    //check for bad address errors
    if (first_blk < 0 || first_blk + num_blks > dev->ops->num_blocks(dev))
        return E_BADADDR;

    int unitSize = r4dev->unitSize;
    int disksCount = r4dev->dataDisksCount;
    int stripeSize = unitSize * disksCount;
    int i;
    for (i = num_blks; i > 0;) {
        int blockInDisk = (first_blk % unitSize) + (first_blk / stripeSize) * unitSize;
        int blocksRemaining = unitSize - (blockInDisk % unitSize);
        int diskNum = (first_blk % stripeSize) / unitSize;

        int len = (i <= blocksRemaining) ? i : blocksRemaining;
        struct blkdev *diskToUse = r4dev->disks[diskNum];
        int returnValue;

        // recovered data for failed disk
        char recoveredDataBuf[len * BLOCK_SIZE];
        if (r4dev->isDegraded)
            recoverFailedDiskData(dev, blockInDisk, len, recoveredDataBuf);

        if (diskToUse != NULL)
            returnValue = diskToUse->ops->write(diskToUse, blockInDisk, len, buf);
        else returnValue = E_UNAVAIL;

        // checking for data disks failures
        // returning E_UNAVAIL if more than 1 disk fails
        if (returnValue == E_UNAVAIL) {

            // if the raid4 device is already degraded
            if (r4dev->isDegraded) {
                // and if the known failed disk is not this disk, return E_UNAVAIL
                if (r4dev->failedDiskNumber != diskNum)
                    return E_UNAVAIL;
                else {
                    // compute parity with new data, other disk data
                    char parityBuf[len * BLOCK_SIZE];
                    returnValue = computeParity(dev, blockInDisk, len, buf, parityBuf);

                    // if a disk fails while computing parity
                    if (returnValue != -1) return E_UNAVAIL;

                    // write parity
                    returnValue = parityDisk->ops->write(parityDisk, blockInDisk, len, parityBuf);

                    // if parity disk fails
                    if (returnValue == E_UNAVAIL) return E_UNAVAIL;
                }
            } else {
                // marking the raid4 device as degraded
                r4dev->isDegraded = 1;
                r4dev->failedDiskNumber = diskNum;

                // compute parity with new data, other disk data
                char parityBuf[len * BLOCK_SIZE];
                returnValue = computeParity(dev, blockInDisk, len, buf, parityBuf);

                // if disk failed while computing parity
                if (returnValue == E_UNAVAIL) return E_UNAVAIL;

                // write parity
                returnValue = parityDisk->ops->write(parityDisk, blockInDisk, len, parityBuf);

                // if parity disk failed
                if (returnValue == E_UNAVAIL) return E_UNAVAIL;
            }

        } else if (returnValue == SUCCESS) {
            // compute parity using data disk values
            char parityBuf[len * BLOCK_SIZE];
            // returnValue = computeParity(dev, blockInDisk, len, NULL, parityBuf);
            returnValue = computeParity(dev, blockInDisk, len, recoveredDataBuf, parityBuf);

            // if a disk fails while computing parity
            if (returnValue != -1) {
                r4dev->isDegraded = 1;
                r4dev->failedDiskNumber = returnValue;
            }

            // write parity
            returnValue = parityDisk->ops->write(parityDisk, blockInDisk, len, parityBuf);

            // if parity disk fails
            if (returnValue == E_UNAVAIL) {
                if (r4dev->isDegraded) return E_UNAVAIL;
                else {
                    r4dev->isDegraded = 1;
                    r4dev->failedDiskNumber = r4dev->dataDisksCount;
                }
            }
        }
        // incrementing buffer, iter variable and the block for next iteration
        buf = buf + len * BLOCK_SIZE;
        i = i - len;
        first_blk = first_blk + len;
    }
    return SUCCESS;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in raid4_create.
 */
static void raid4_close(struct blkdev * dev) {
    struct raid4_dev *r4dev = dev->private;
    int i;
    for (i = 0; i < r4dev->dataDisksCount + 1; i++) {
        struct blkdev *disk = r4dev->disks[i];
        disk->ops->close(disk);
    }
    free(r4dev);
    free(dev);
}

/* Overriding the basic operations of a block device for raid0 */
struct blkdev_ops raid4_ops = {
    .num_blocks = raid4_num_blocks,
    .read = raid4_read,
    .write = raid4_write,
    .close = raid4_close
};


/* Initialize a RAID 4 volume with strip size 'unit', using
 * disks[N-1] as the parity drive. Do not write to the disks - assume
 * that they are properly initialized with correct parity. (warning -
 * some of the grading scripts may fail if you modify data on the
 * drives in this function)
 */
struct blkdev *raid4_create(int N, struct blkdev * disks[], int unit) {
    if (checkDisksSameSize(N, disks) == -1)
        return NULL;

    int num_blocks = disks[0]->ops->num_blocks(disks[0]);
    struct blkdev *dev = malloc(sizeof(*dev));
    struct raid4_dev *r4dev = malloc(sizeof(*r4dev));

    r4dev->disks = disks;
    r4dev->parityDisk = disks[N - 1];
    r4dev->dataDisksCount = N - 1;
    r4dev->usableBlocksPerDisk = (num_blocks / unit) * unit;
    r4dev->unitSize = unit;
    r4dev->isDegraded = 0;
    r4dev->failedDiskNumber = -1;
    dev->private = r4dev;
    dev->ops = &raid4_ops;

    return dev;
}


/* replace failed device 'i' in a RAID 4. Note that we assume
 * the upper layer knows which device failed. You will need to
 * reconstruct content from data and parity before returning
 * from this call.
 */
int raid4_replace(struct blkdev *volume, int i, struct blkdev * newdisk) {
    struct raid4_dev *r4dev = volume->private;
    struct blkdev *diskToBeReplaced = r4dev->disks[i];

    // setting the disk to be replaced as the failed disk
    r4dev->failedDiskNumber = i;

    int r4DiskBlockCount = diskToBeReplaced->ops->num_blocks(diskToBeReplaced);
    if (newdisk->ops->num_blocks(newdisk) != r4DiskBlockCount)
        return E_SIZE;

    // Compute parity from other disks and store in new disk
    // irrespective of it being a data drive or a parity drive
    char recoveredDataBuf[r4DiskBlockCount * BLOCK_SIZE];
    recoverFailedDiskData(volume, 0, r4DiskBlockCount, recoveredDataBuf);

    // write computed data into the newdisk
    newdisk->ops->write(newdisk, 0, r4DiskBlockCount, recoveredDataBuf);

    // Closing the disk to be replaced
    diskToBeReplaced->ops->close(diskToBeReplaced);
    r4dev->disks[i] = newdisk;

    return SUCCESS;
}