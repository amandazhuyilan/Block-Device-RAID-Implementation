Run shell files (.sh) to see testing results. 

## Block Device RAID 0/1/4 Implementation

In this assignment, we are implenting RAID 1(mirroring), RAID 0(Stripping) and RAID4 in a block device layer.

#### Some basic provided stuff
- In the  ```blkdev.h``` header file, ```blkdev``` is structured with a pointer ```*ops```that points to the basic operations of a block device, while the ```private``` points to the block device itself. Therefore whenever we are initalizing a new block device, we reference it as (passing on ```blkdev *dev``` as input) ```struct new_device *new_dev = dev->private```.

#### Mirroring (RAID 1):

```mirror_num_blocks(struct blkdev *dev)``` returns the number of blocks in of the mirrored device.

```mirror_read(struct blk_dev *dev, int first_blk, int num_blks, void *buf)``` reads the ```len``` sectors into the buffer for the two disks of the mirror device.
First checks if ```first_blk + num_blks``` exceeds the size of the mirror device (if exceeds return ```E_BADADDR```), while the disk is not NULL, implement the ```read``` function in ```blkdev_ops```. Returns ```SUCCESS``` if read successfully, otherwise return ```E_UNAVAIL```.

```mirror_create(struct blkdev *disks[2])``` copy the contents of both disks assuming the are of the same size (will return error message if not of the same) to a mirror device's disks. Also copies the size of the disk to the mirrored device's ```nblks```.

```mirror_write(struct blkdev * dev, int first_blk,int num_blks, void *buf)``` writes to both disks of mirror or the remaining good side if the other one has a bad read or is NULL. Will return SUCCESS as long as one read is successful, otherwise return E_UNAVIL.

```mirror_close(struct blkdev *dev)``` closes previously opened non-failed disks, and ```free``` any ```malloc```ed space for blkdev structures.
 
```mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)``` returns ```E_SIZE`` if the size of ```newdisk``` is not as the same size of ```volume```. The non-failing disk need to be replicate using ```read``` to working disk and ```write``` to ```newdisk```. Will return SUCCESS upen coping failed disk to ```newdisk```.
 
 
 #### Test Cases for RAID 1:
 
1. Creates a volume properly.

2. Returns the correct length.

3. Can handle reads and writes of different sizes, and return the same data as written.

4. Reads data from the proper location in the images, and doesnt overwrite incorrect locations. on write.

5. Continues to read and write correctly after one of the disks fails.

6. Continues to read and write (correctly returning data written before the failure) after the disk is replaced.

7. Reads and writes (returning data written before first failure) after the other disk fails.
 
#### Striping (RAID 0):
```struct raid0_dev``` is a custom struct for raid0 devices similar to block dev and image dev.

```struct blkdev *raid0_create(int N, struct blkdev *disks[], int unit)``` creates a raid0 type block device. Returns NULL incase of invalid no. of disks or if the disks are unequal in size.

```int raid0_num_blocks(struct blkdev *dev)``` returns the size of the active stripes in the disk which is an exact multiple of the unit size of a stripe.

```static int raid0_read_write(struct blkdev * dev, int first_blk, int num_blks, void *buf, int isWrite)``` a generic function for both read and write operations into the raid0 type block device. Calculates the disk number and blocks to be read from to written to.

```static int raid0_write(struct blkdev * dev, int first_blk, int num_blks, void *buf)``` calls the generic function ```raid0_read_write``` with write flag enabled.

```static int raid0_read(struct blkdev * dev, int first_blk, int num_blks, void *buf)``` calls the generic function ```raid0_read_write``` with write flag disabled.

```static void raid0_close(struct blkdev *dev)``` closes all underlying discs in this raid0_dev structure. Similar to what was implemented in mirror_close().


#### RAID 4:
```struct raid4_dev``` is a custom struct for raid4 devices similar to block dev and image dev.

```struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)``` creates a raid4 type block device. Returns NULL incase of invalid no. of disks or if the disks are unequal in size. Initializes the logical construct of parity and data disks into the raid4_dev structure.

```static void raid4_close(struct blkdev *dev)``` closes all underlying discs in this raid4_dev structure. Similar to what was implemented in raid0_close().

```int raid4_num_blocks(struct blkdev *dev)``` returns the size of the active stripes in the data disk (not including the parity disc) which is an exact multiple of the unit size of a stripe.

```static int raid4_read(struct blkdev * dev, int first_blk, int num_blks, void *buf)``` implemented raid4_read using the following logic:
				read normal:
					read logic similar to raid0
				read degrad:
					recover failed disk's data using XOR on all other disks

```static int raid4_write(struct blkdev *dev, int first_blk, int num_blks, void *buf)``` implemented raid4_write using the following logic:
				write normal:
					compute parity 
					write parity
					write data on the data disk
				write degrad:
					if disk-failed:
						compute parity with new data, other disk data
						write parity
					if disk-normal:
						recover failed disk's data using XOR on all other disks
						compute parity using recovered as well as other data
						write parity
						write data on to the normal disk

```int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)``` copies computed parity data from the rest of the disks in the RAID4 device into the newdisk and replaces disk i with the newdisk.
