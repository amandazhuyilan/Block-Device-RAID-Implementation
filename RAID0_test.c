#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "blkdev.h"

// Requirements for RAID 0 testing
// 1. Passes all other tests with different strip size (e.g 2,4,7 and 32 sectors) and different number of disks
// 2. Reports the correct size 
// 3. Reads data from the right disks and locations. (prepare disks with known data at various locations and make sure you can read it back)
//4. Overwrites the correct location. (Write to to your prepared disks and check the results- using something other than your stripe code -  to check that the write sections that got modified)
//5. Fails a disk and verify that volume fails
//6. large (>1 stripe set), small, unaligned read and writes (i.e starting, ending in the middle of a stripe), as well as small writes wrapping around the end of a stripe.

int main(int argc, char **argv)
{

    struct blkdev *disks[5];
    int i;
    int num_disks = 0;				//ndisks = argc = the total number of input +1
    int stripe_size = atoi(argv[1]);

    for (i = 2; i < argc; i++)
        disks[num_disks++] = image_create(argv[i]);

    struct blkdev *RAID_0 = raid0_create(num_disks, disks, stripe_size);
    assert(RAID_0 != NULL);


 	int nblks = disks[0]->ops->num_blocks(disks[0]);
    nblks = nblks - (nblks % stripe_size);
    assert(RAID_0->ops->num_blocks(RAID_0) == num_disks*nblks);

    int one_chunk = stripe_size * BLOCK_SIZE;
    char *buf = malloc(num_disks * one_chunk);
    for (i = 0; i < num_disks; i++)
        memset(buf + i * one_chunk, 'A'+i, one_chunk);

    int result, j;
    for (i = 0; i < 16; i++) {
        result = RAID_0->ops->write(RAID_0, i*num_disks*stripe_size,
                                     num_disks*stripe_size, buf);
        assert(result == SUCCESS);
    }

    char *buf2 = malloc(num_disks * one_chunk);

    for (i = 0; i < 16; i++) {
        result = RAID_0->ops->read(RAID_0, i*num_disks*stripe_size,
                                    num_disks*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, num_disks * one_chunk) == 0);
    }

    /* now we test that the data got onto the disks in the right
     * places. 
     */
    for (i = 0; i < num_disks; i++) {
        result = disks[i]->ops->read(disks[i], i*stripe_size,
                                     stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf + i*one_chunk, buf2, one_chunk) == 0);
    }

    /* now we test that small writes work
     */
    for (i = 0; i < num_disks; i++)
        memset(buf + i * one_chunk, 'a'+i, one_chunk);
    
    for (i = 0; i < 8; i++) {
        for (j = 0; j < num_disks*stripe_size; j ++) {
            result = RAID_0->ops->write(RAID_0, i*num_disks*stripe_size + j, 1,
                                         buf + j*BLOCK_SIZE);
            assert(result == SUCCESS);
        }
    }

    for (i = 0; i < 8; i++) {
        result = RAID_0->ops->read(RAID_0, i*num_disks*stripe_size,
                                    num_disks*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, num_disks * one_chunk) == 0);
    }

    /* finally we test that large and overlapping writes work.
     */
    char *big = malloc(5 * num_disks*one_chunk);
    for (i = 0; i < 5; i++)
        for (j = 0; j < num_disks; j++)
            memset(big + j * one_chunk + i*num_disks*one_chunk, 'f'+i, one_chunk);

    int offset = num_disks*stripe_size / 2;
    result = RAID_0->ops->write(RAID_0, offset, 5*num_disks*stripe_size, big);
    assert(result == SUCCESS);

    char *big2 = malloc(5 * num_disks*one_chunk);
    result = RAID_0->ops->read(RAID_0, offset, 5*num_disks*stripe_size, big2);
    assert(result == SUCCESS);
    assert(memcmp(big, big2, 5 * num_disks * one_chunk) == 0);

    /* and check we didn't muck up any previously-written data
     */
    result = RAID_0->ops->read(RAID_0, 0, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf, buf2, offset*BLOCK_SIZE) == 0);

    result = RAID_0->ops->read(RAID_0, 5*num_disks*stripe_size + offset, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf + offset*BLOCK_SIZE, buf2, offset*BLOCK_SIZE) == 0);
    
    printf("Striping Test: SUCCESS\n");
    return 0;
}

    /* your tests here */