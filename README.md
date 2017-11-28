### Homework 2 Block Devices

In this assignment, we are implenting RAID0(mirroring), RAID1(Stripping) and RAID4 in a block device layer.

#### Some basic provided stuff
- In the  ```blkdev.h``` header file, ```blkdev``` is structured with a pointer ```*ops```that points to the basic operations of a block device, while the ```private``` points to the block device itself. Therefore whenever we are initalizing a new block device, we reference it as (passing on ```blkdev *dev``` as input) ```struct new_device *new_dev = dev->private```.

#### Mirroring (RAID 0):

```mirror_num_blocks(struct blkdev *dev)``` returns the number of blocks in of the mirrored device.

```mirror_read(struct blk_dev *dev, int first_blk, int num_blks, void *buf)``` reads the ```len``` sectors into the buffer for the two disks of the mirror device.
First checks if ```first_blk + num_blks``` exceeds the size of the mirror device (if exceeds return ```E_BADADDR```), while the disk is not NULL, implement the ```read``` function in ```blkdev_ops```. Returns ```SUCCESS``` if read successfully, otherwise return ```E_UNAVAIL```.

```mirror_create(struct blkdev *disks[2])``` copy the contents of both disks assuming the are of the same size (will return error message if not of the same) to a mirror device's disks. Also copies the size of the disk to the mirrored device's ```nblks```.

```mirror_write(struct blkdev * dev, int first_blk,int num_blks, void *buf)``` writes 
