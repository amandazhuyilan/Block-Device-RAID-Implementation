#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "blkdev.h"

// Requirements for mirror tests:
// 1. Creates a volume properly
// 2. Returns the correct length
// 3. Can handle reads and writes of different sizes, and return the same data as written
// 4. reads data from the proper location in the images, and doesnt overwrite incorrect locations on write.
// 5. Continues to read and write correctly after one of the disks fails
// 6. Continues to read and write (correctly returning data written before the failure) after the disk is replaced.
// 7. Reads and writes (returning data written before first failure) after the other disk fails

int main(int arg, char * argv[]){
	// Amanda: create blkdevs for disks images using functions from image.c
	struct blkdev *disk_1 = image_create(argv[1]);
	struct blkdev *disk_2 = image_create(argv[2]);

	//Test 1. Creating new mirror properly
	struct blkdev *new_disks[2] = {disk_1, disk_2};
	struct blkdev *mirror = mirror_create(new_disks);

	// asserts that a mirror created successfully 
	assert(mirror != NULL);
	// Test 2: Returns the correct length
	assert(mirror->ops->num_blocks(mirror) == disk_1->ops->num_blocks(disk_1));

	// read and write with block_num = 2
	// test_write and test_read will be reused frequently in the following tests
	int block_num = 2;
	char test_read [block_num * BLOCK_SIZE];
	char test_write [block_num * BLOCK_SIZE];

	// setting the first block_num * BLOCK_SIZE of test_write into char 'A'
	memset(test_write, 'A', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num, test_read)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write, test_read, block_num * BLOCK_SIZE ) == 0 );

	//Test 3. Can handle reads and writes of different sizes, and return the same data as written
	// read and write with different sizes block_num = 4
	int block_num_1 = 4;
	char test_read_1 [block_num_1 * BLOCK_SIZE];
	char test_write_1 [block_num_1 * BLOCK_SIZE];

	// setting the first block_num * BLOCK_SIZE of test_write into char 'A'
	memset(test_write, 'B', block_num_1 * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num_1, test_write_1)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num_1, test_read_1)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write_1, test_read_1, block_num_1 * BLOCK_SIZE ) == 0 );

	//Test 4. reads data from the proper location in the images, and doesnt overwrite incorrect locations on write.
	// reading and writing to incorrect address (LBA<0 or first_blk+num_blks>mdev->nblks)
	 assert ( mirror -> ops -> read ( mirror, -2, block_num, test_read )
         == E_BADADDR );
	 assert ( mirror -> ops -> read ( mirror, 2, 20, test_read )
         == E_BADADDR );

	 assert ( mirror -> ops -> write ( mirror, -2, block_num, test_write )
             == E_BADADDR );
 	 assert ( mirror -> ops -> write ( mirror, 2, 20, test_read )
     == E_BADADDR );


	 //Test 5. Continues to read and write correctly after one of the disks fails
	 image_fail ( disk_1 );
	 memset(test_write, 'F', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num, test_read)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write, test_read, block_num * BLOCK_SIZE ) == 0 );

	//Test 6. Continues to read and write (correctly returning data written before the failure) after the disk is replaced.
	// create a new image and check if vaild, and if size is same as the failed disk_1
	struct blkdev *new_disk = image_create("mirror_new_disk.img");
	assert(new_disk != NULL);
	assert(new_disk->ops->num_blocks(new_disk) == disk_1->ops->num_blocks(disk_1));

	// Check if contents are copied to new_disk
	assert ( mirror_replace ( mirror, 0, new_disk ) == SUCCESS );
	assert(mirror->ops->read(mirror, 0, block_num, test_read) == SUCCESS);

	// Test 7. Reads and writes (returning data written before first failure) after the other disk fails

	image_fail(disk_2);
	memset(test_write, 'Y', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num, test_read)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write, test_read, block_num * BLOCK_SIZE ) == 0 );

	//Fail new_disk, read and write should be E_UNAVAIL
	image_fail(new_disk);
	assert(mirror->ops->write(mirror, 0, block_num, test_write)==E_UNAVAIL);
	assert(mirror->ops->read(mirror, 0, block_num, test_read)==E_UNAVAIL);
	mirror -> ops -> close ( mirror );
    printf("Mirror Test Completed!\n");
    return 0;

}
