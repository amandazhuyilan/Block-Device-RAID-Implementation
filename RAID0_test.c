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
    int i, ndisks;
    int stripesize = atoi(argv[1]);

    printf("i = %d, ndisks = %d, stripeszie = %d,", i, ndisks, stripesize);

    for (i = 2, ndisks = 0; i < argc; i++)
        disks[ndisks++] = image_create(argv[i]);

    /* and creates a striped volume with the specified stripe size
     */
    struct blkdev *striped = raid0_create(ndisks, disks, stripesize);
    assert(striped != NULL);

    /* your tests here */

}