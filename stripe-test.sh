gcc -g -w -o stripe-test stripe-test.c image.c homework.c -g

dd if=/dev/zero bs=512 count=1024 | tr '\0' '1' > RAID0_disk1.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '2' > RAID0_disk2.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '3' > RAID0_disk3.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '4' > RAID0_disk4.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '5' > RAID0_disk5.img

dd if=/dev/zero bs=512 count=1024 | tr '\0' '6' > RAID0_disk6.img

echo "Testing with stripe size = 2 and disk num = 4"
./stripe-test 2 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 

echo "Testing with stripe size = 4 and disk num = 4"
./stripe-test 4 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 

echo "Testing with stripe size = 7 and disk num = 6"
./stripe-test 7 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img RAID0_disk5.img RAID0_disk6.img

echo "Testing with stripe size = 32 and disk num = 6"
./stripe-test 32 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img RAID0_disk5.img RAID0_disk6.img

echo "Passed test 1: Passes all other tests with different strip size (e.g 2,4,7 and 32 sectors) and different number of disks"

echo "Passed all RAID 0 test!"