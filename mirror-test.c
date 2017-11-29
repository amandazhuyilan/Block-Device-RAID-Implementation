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
	char test_read_2 [block_num * BLOCK_SIZE];
	char test_write_2 [block_num * BLOCK_SIZE];

	// setting the first block_num * BLOCK_SIZE of test_write into char 'A'
	memset(test_write_2, 'A', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write_2)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num, test_read_2)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write_2, test_read_2, block_num * BLOCK_SIZE ) == 0 );

	printf("Completed Test 1 & 2: creating a mirror and return correct length.\n");

	//Test 3. Can handle reads and writes of different sizes, and return the same data as written
	// read and write with different sizes block_num = 4
	int block_num_1 = 4;
	char test_read_3 [block_num_1 * BLOCK_SIZE];
	char test_write_3 [block_num_1 * BLOCK_SIZE];

	// setting the first block_num * BLOCK_SIZE of test_write into char 'A'
	memset(test_write_3, 'B', block_num_1 * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num_1, test_write_3)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num_1, test_read_3)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write_3, test_read_3, block_num_1 * BLOCK_SIZE ) == 0 );

	printf("Completed Test 3:  Can handle reads and writes of different sizes, and return the same data as written.\n");

	//Test 4. reads data from the proper location in the images, and doesnt overwrite incorrect locations on write.
	// reading and writing to incorrect address (LBA<0 or first_blk+num_blks>mdev->nblks)
	char test_read_4 [block_num * BLOCK_SIZE];
	char test_write_4 [block_num * BLOCK_SIZE];

	 assert ( mirror -> ops -> read ( mirror, -2, block_num, test_read_4 )
         == E_BADADDR );

	 assert ( mirror -> ops -> read ( mirror, 2, 20, test_read_4 )
         == E_BADADDR );

	 assert ( mirror -> ops -> write ( mirror, -2, block_num, test_write_4 )
             == E_BADADDR );
 	 assert ( mirror -> ops -> write ( mirror, 2, 20, test_read_4 )
     == E_BADADDR );

     printf("Completed Test 4: reads data from the proper location in the images, and doesnt overwrite incorrect locations on write.\n");

	 //Test 5. Continues to read and write correctly after one of the disks fails
	 image_fail(disk_2);

	 char test_write_5 [block_num * BLOCK_SIZE];
	 char test_read_5 [block_num * BLOCK_SIZE];
	 memset(test_write_5, 'F', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write_5)==SUCCESS);

	assert(mirror->ops->read(mirror, 0, block_num, test_read_5)==SUCCESS);

	//compare read and write values
	assert ( strncmp( test_write_5, test_read_5, block_num * BLOCK_SIZE ) == 0 );

	printf("Completed Test 5: Continues to read and write correctly after one of the disks fails\n");

	//Test 6. Continues to read and write (correctly returning data written before the failure) after the disk is replaced.
	// create a new image and check if vaild, and if size is same as the failed disk_1
	struct blkdev *new_disk = image_create("mirror_new_disk.img");
	assert(new_disk != NULL);
	assert(new_disk->ops->num_blocks(new_disk) == disk_1->ops->num_blocks(disk_1));

	// Check if contents are copied to new_disk
	assert ( mirror_replace ( mirror, 1, new_disk ) == SUCCESS );
	char test_read_6 [block_num * BLOCK_SIZE];
	assert(mirror->ops->read(mirror, 1, block_num, test_read_6) == SUCCESS);

	printf("Completed Test 6: Continues to read and write (correctly returning data written before the failure) after the disk is replaced.\n");

	// Test 7. Reads and writes (returning data written before first failure) after the other disk fails

	image_fail(disk_1);
	printf("failed disk 1");
	char test_write_7 [block_num * BLOCK_SIZE];
	char test_read_7 [block_num * BLOCK_SIZE];
	memset(test_write_7, 'Y', block_num * BLOCK_SIZE);

	assert(mirror->ops->write(mirror, 0, block_num, test_write_7)==SUCCESS);
	assert(mirror->ops->read(mirror, 0, block_num, test_read_7)==SUCCESS);
	//compare read and write values
	assert ( strncmp( test_write_7, test_read_7, block_num * BLOCK_SIZE ) == 0 );

	//Fail new_disk, read and write should be E_UNAVAIL
	image_fail(new_disk);
	assert(mirror->ops->write(mirror, 0, block_num, test_write_7)==E_UNAVAIL);
	assert(mirror->ops->read(mirror, 0, block_num, test_read_7)==E_UNAVAIL);

	printf("Completed Test 7: CReads and writes (returning data written before first failure) after the other disk fails.\n");

	mirror -> ops -> close ( mirror );
    printf("Mirror Test Completed!\n");
    return 0;

}
