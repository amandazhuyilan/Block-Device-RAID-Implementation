// Requirements for RAID4 tests:
// 1. Creates a volume properly, passes all other tests with different strip size (e.g 2,4,7 and 32 sectors) and different number of disks.
// 2. Returns the correct length
// 3. Can handle reads and writes of different sizes, and return the same data as written
// 4. reads data from the proper location in the images, and doesnt overwrite incorrect locations on write.
// 5. Continues to read and write correctly after one of the disks fails
// 6. Continues to read and write (correctly returning data written before the failure) after the disk is replaced.
// 7. Reads and writes (returning data written before first failure) after the other disk fail
//8. Large (>1 stripe set), small, unaligned read and writes (i.e starting, ending in the middle of a stripe), as well as small writes wrapping around the end of a stripe.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "blkdev.h"

int main(int argc, char **argv)
{
    struct blkdev *disks[5];
    int i;
    int num_disks = 0;				//num_disks = argc = the total number of input +1
    int stripe_size = atoi(argv[1]);

    for (i = 2; i < argc; i++){
    	disks[num_disks] = image_create(argv[i]);
    	num_disks++;	
    }

    struct blkdev *RAID_4 = raid4_create(num_disks, disks, stripe_size);
    assert(RAID_4 != NULL);

    int nblks = disks[0]->ops->num_blocks(disks[0]);
    nblks = nblks - (nblks % stripe_size);
    assert(RAID_4->ops->num_blocks(RAID_4) == (num_disks -1)*nblks);

    printf("Passed test 2: Reports the correct size.\n");

    int chunk = stripe_size * BLOCK_SIZE;
    //char *buf = malloc(num_disks * chunk);
    char *buf = calloc((num_disks-1) * chunk + 100, 1);

    for (i = 0; i < num_disks-1; i++)
        memset(buf + i * chunk, 'A'+i, chunk);

    int result;
    for (i = 0; i < 10; i++) {
        result = RAID_4->ops->write(RAID_4, i*(num_disks -1)*stripe_size,
                                     (num_disks-1)*stripe_size, buf);
        assert(result == SUCCESS);
    }

    char *buf2 = malloc(num_disks * chunk);

    for (i = 0; i < 10; i++) {
        result = RAID_4->ops->read(RAID_4, i*num_disks*stripe_size,
                                    num_disks*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, (num_disks -1) * chunk) == 0);
    }

    int j;
  	for (i = 0; i < (num_disks -1); i++) {
   		for (j = 0; j < 10; j++) {
   			result = disks[i]->ops->read(disks[i], i*stripe_size,
                                     stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf + i*chunk, buf2, chunk) == 0);
    	}
    }

    printf("Passed test 3: Writes and read back back data from the right disks and locations\n.");

    for (i = 0; i < (num_disks -1); i++)
        memset(buf + i * chunk, 'a'+i, chunk);
    
    for (i = 0; i < 8; i++) {
        for (j = 0; j < (num_disks -1)*stripe_size; j ++) {
            result = RAID_4->ops->write(RAID_4, i*(num_disks-1)*stripe_size + j, 1,
                                         buf + j*BLOCK_SIZE);
            assert(result == SUCCESS);
        }
    }

    for (i = 0; i < 8; i++) {
        result = RAID_4->ops->read(RAID_4, i*(num_disks-1)*stripe_size,
                                    (num_disks-1)*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, (num_disks -1) * chunk) == 0);
    }

    printf("Passed small writes. \n");

        char *big = malloc(5 * (num_disks -1)*chunk +100);
    for (i = 0; i < 5; i++)
        for (j = 0; j < (num_disks -1); j++)
            memset(big + j * chunk + i*(num_disks -1)*chunk, 'f'+i, chunk);

    int offset = (num_disks -1) * stripe_size / 2;
    result = RAID_4->ops->write(RAID_4, offset, 5*(num_disks -1)*stripe_size, big);
    assert(result == SUCCESS);

    char *big2 = malloc(5 * (num_disks -1)*chunk + 100);
    result = RAID_4->ops->read(RAID_4, offset, 5*(num_disks -1)*stripe_size, big2);
    assert(result == SUCCESS);
    assert(memcmp(big, big2, 5 * (num_disks -1) * chunk) == 0);


    result = RAID_4->ops->read(RAID_4, 0, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf, buf2, offset*BLOCK_SIZE) == 0);

    result = RAID_4->ops->read(RAID_4, 5*(num_disks -1)*stripe_size + offset, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf + offset*BLOCK_SIZE, buf2, offset*BLOCK_SIZE) == 0);

   	printf("Passed test 8. Large (>1 stripe set), small, unaligned read and writes. \n");

   	image_fail(disks[0]);

    result = RAID_4->ops->read(RAID_4, offset, 5*(num_disks-1)*stripe_size, big2);
    assert(result == SUCCESS);
    assert(memcmp(big, big2, 5*(num_disks-1)*chunk) == 0);

    for (i = 0; i < (num_disks -1); i++)
        memset(buf + i * chunk, 'g'+i, chunk);

    for (i = 0; i < 10; i++) {
        result = RAID_4->ops->write(RAID_4, i*(num_disks -1)*stripe_size,
                                  (num_disks -1)*stripe_size, buf);
        assert(result == SUCCESS);
    }

    for (i = 0; i < 10; i++) {
        memset(buf2, 0, (num_disks -1)*stripe_size*BLOCK_SIZE);
        result = RAID_4->ops->read(RAID_4, i*(num_disks-1)*stripe_size,
                                 (num_disks-1)*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, (num_disks-1) * chunk) == 0);
    }

    printf("Passed test 5: Continues to read and write correctly after one of the disks fails");

     struct blkdev *new_disk = disks[num_disks-1];
	 result = raid4_replace(RAID_4, 0, new_disk);
	        assert(result == SUCCESS);
	    
	        for (i = 0; i < 10; i++) {
	            memset(buf2, 0, num_disks*stripe_size*BLOCK_SIZE);
	            result = RAID_4->ops->read(RAID_4, i*(num_disks-1)*stripe_size,
	                                     (num_disks-1)*stripe_size, buf2);
	            assert(result == SUCCESS);
	            assert(memcmp(buf, buf2, (num_disks-1) * chunk) == 0);
	        }

	        // create a new disk for replacement
	        image_fail(disks[1]);
	        buf[0] = 'h';
	        for (i = 0; i < 10; i++) {
	            RAID_4->ops->write(RAID_4, i*(num_disks-1)*stripe_size,
	                             (num_disks-1)*stripe_size, buf);
	            assert(result == SUCCESS);
	        }
	    
	        for (i = 0; i < 10; i++) {
	            result = RAID_4->ops->read(RAID_4, i*(num_disks-1)*stripe_size,
	                                     (num_disks-1)*stripe_size, buf2);
	            assert(result == SUCCESS);
	            assert(memcmp(buf, buf2, (num_disks-1) * chunk) == 0);
	        }

	   	printf("RAID4 Test: SUCCESS\n");
	    }


