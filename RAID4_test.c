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

    struct blkdev *RAID_4 = raid0_create(num_disks, disks, stripe_size);
    assert(RAID_4 != NULL);

    int nblks = disks[0]->ops->num_blocks(disks[0]);
    nblks = nblks - (nblks % stripe_size);
    assert(RAID_4->ops->num_blocks(RAID_4) == num_disks*nblks);

    printf("Passed test 2: Reports the correct size.\n");


}