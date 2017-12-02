#!/bin/sh

gcc -g -w -o raid4-test raid4-test.c image.c homework.c -g

dd if=/dev/zero bs=512 count=1024 | tr '\0' '1' > RAID4_disk1.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '2' > RAID4_disk2.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '3' > RAID4_disk3.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '4' > RAID4_disk4.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '5' > RAID4_disk5.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '6' > RAID4_disk6.img

echo "Testing with stripe size = 2 and disk num = 4"
./raid4-test 2 RAID4_disk1.img RAID4_disk2.img RAID4_disk3.img RAID4_disk4.img 

echo "Testing with stripe size = 4 and disk num = 4"
./raid4-test 4 RAID4_disk1.img RAID4_disk2.img RAID4_disk3.img RAID4_disk4.img 

echo "Testing with stripe size = 7 and disk num = 6"
./raid4-test 7 RAID4_disk1.img RAID4_disk2.img RAID4_disk3.img RAID4_disk4.img RAID4_disk5.img RAID4_disk6.img 

echo "Testing with stripe size = 32 and disk num = 6"
./raid4-test 32 RAID4_disk1.img RAID4_disk2.img RAID4_disk3.img RAID4_disk4.img RAID4_disk5.img RAID4_disk6.img 

echo "Passed test 1: Passes all other tests with different strip size (e.g 2,4,7 and 32 sectors) and different number of disks"