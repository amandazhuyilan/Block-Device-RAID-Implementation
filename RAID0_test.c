// Requirements for RAID 0 testing
// 1. Passes all other tests with different strip size (e.g 2,4,7 and 32 sectors) and different number of disks
// 2. Reports the correct size 
// 3. Reads data from the right disks and locations. (prepare disks with known data at various locations and make sure you can read it back)
//4. Overwrites the correct location. (Write to to your prepared disks and check the results- using something other than your stripe code -  to check that the write sections that got modified)
//5. Fails a disk and verify that volume fails
//6. large (>1 stripe set), small, unaligned read and writes (i.e starting, ending in the middle of a stripe), as well as small writes wrapping around the end of a stripe.

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

    struct blkdev *RAID_0 = raid0_create(num_disks, disks, stripe_size);
    assert(RAID_0 != NULL);

    int nblks = disks[0]->ops->num_blocks(disks[0]);
    nblks = nblks - (nblks % stripe_size);
    assert(RAID_0->ops->num_blocks(RAID_0) == num_disks*nblks);

    printf("Passed test 2: Reports the correct size.\n");

    int chunk = stripe_size * BLOCK_SIZE;
    char *buf = malloc(num_disks * chunk);
    for (i = 0; i < num_disks; i++)
        memset(buf + i * chunk, 'A'+i, chunk);

    int result, j;
    for (i = 0; i < 10; i++) {
        result = RAID_0->ops->write(RAID_0, i*num_disks*stripe_size,
                                     num_disks*stripe_size, buf);
        assert(result == SUCCESS);
    }

    char *buf2 = malloc(num_disks * chunk);

    for (i = 0; i < 10; i++) {
        result = RAID_0->ops->read(RAID_0, i*num_disks*stripe_size,
                                    num_disks*stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, num_disks * chunk) == 0);
    }

    for (i = 0; i < num_disks; i++) {
        result = disks[i]->ops->read(disks[i], i*stripe_size,
                                     stripe_size, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf + i*chunk, buf2, chunk) == 0);
    }


    printf("Passed test 3: Writes and read back back data from the right disks and locations\n.");

    for (i = 0; i < num_disks; i++)
        memset(buf + i * chunk, 'a'+i, chunk);
    
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
        assert(memcmp(buf, buf2, num_disks * chunk) == 0);
    }

    printf("Passed test 4. Overwrites the correct location. \n");

    image_fail(disks[0]);
    assert(RAID_0 ->ops ->read(RAID_0, 0, 1, buf) == E_UNAVAIL);

   	printf("Passed test 5. Fails a disk and verify that volume fails\n");

    char *big = malloc(5 * num_disks*chunk);
    for (i = 0; i < 5; i++)
        for (j = 0; j < num_disks; j++)
            memset(big + j * chunk + i*num_disks*chunk, 'f'+i, chunk);

    int offset = num_disks*stripe_size / 2;
    result = RAID_0->ops->write(RAID_0, offset, 5*num_disks*stripe_size, big);
    assert(result == SUCCESS);

    char *big2 = malloc(5 * num_disks*chunk);
    result = RAID_0->ops->read(RAID_0, offset, 5*num_disks*stripe_size, big2);
    assert(result == SUCCESS);
    assert(memcmp(big, big2, 5 * num_disks * chunk) == 0);


    result = RAID_0->ops->read(RAID_0, 0, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf, buf2, offset*BLOCK_SIZE) == 0);

    result = RAID_0->ops->read(RAID_0, 5*num_disks*stripe_size + offset, offset, buf2);
    assert(result == SUCCESS);
    assert(memcmp(buf + offset*BLOCK_SIZE, buf2, offset*BLOCK_SIZE) == 0);

   	printf("Passed test 6. Large (>1 stripe set), small, unaligned read and writes. \n");

   
    printf("Striping Test: SUCCESS\n");
    return 0;
}
