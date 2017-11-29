### Homework 2 Block Devices

In this assignment, we are implenting RAID0(mirroring), RAID1(Stripping) and RAID4 in a block device layer.

#### Some basic provided stuff
- In the  ```blkdev.h``` header file, ```blkdev``` is structured with a pointer ```*ops```that points to the basic operations of a block device, while the ```private``` points to the block device itself. Therefore whenever we are initalizing a new block device, we reference it as (passing on ```blkdev *dev``` as input) ```struct new_device *new_dev = dev->private```.

#### Mirroring (RAID 0):

```mirror_num_blocks(struct blkdev *dev)``` returns the number of blocks in of the mirrored device.

```mirror_read(struct blk_dev *dev, int first_blk, int num_blks, void *buf)``` reads the ```len``` sectors into the buffer for the two disks of the mirror device.
First checks if ```first_blk + num_blks``` exceeds the size of the mirror device (if exceeds return ```E_BADADDR```), while the disk is not NULL, implement the ```read``` function in ```blkdev_ops```. Returns ```SUCCESS``` if read successfully, otherwise return ```E_UNAVAIL```.

```mirror_create(struct blkdev *disks[2])``` copy the contents of both disks assuming the are of the same size (will return error message if not of the same) to a mirror device's disks. Also copies the size of the disk to the mirrored device's ```nblks```.

```mirror_write(struct blkdev * dev, int first_blk,int num_blks, void *buf)``` writes to both disks of mirror or the remaining good side if the other one has a bad read or is NULL. Will return SUCCESS as long as one read is successful, otherwise return E_UNAVIL.
 
 ```mirror_close(struct blkdev *dev)``` closes previously opened non-failed disks, and ```free``` any ```malloc```ed space for blkdev structures.
 
 ```mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)``` returns ```E_SIZE`` if the size of ```newdisk``` is not as the same size of ```volume```. The non-failing disk need to be replicate using ```read``` to working disk and ```write``` to ```newdisk```. Will return SUCCESS upen coping failed disk to ```newdisk```.
 
 
 #### Test Cases for Mirroring
 
 
 
 #### Striping (RAID 0):
 ```struct raid0_dev``` is a custom struct for raid0 devices similar to block dev.
 
 ```int raid0_num_blocks(struct blkdev *dev)``` returns the size of the active stripes in the disk which is an exact multiple of the unit size of a stripe.
 
 ```struct blkdev *raid0_create(int N, struct blkdev *disks[], int unit)``` creates a raid0 type block device. Returns NULL incase of invalid no. of disks or if the disks are unequal in size.
