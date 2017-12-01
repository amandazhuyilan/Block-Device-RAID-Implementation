gcc -g -w -o RAID0-test RAID0_test.c image.c homework.c -g

dd if=/dev/zero bs=512 count=10 | tr '\0' '1' > RAID0_disk1.img

dd if=/dev/zero bs=512 count=10 | tr '\0' '2' > RAID0_disk2.img

dd if=/dev/zero bs=512 count=10 | tr '\0' '3' > RAID0_disk3.img

dd if=/dev/zero bs=512 count=10 | tr '\0' '4' > RAID0_disk4.img


./RAID0-test 2 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 

./RAID0-test 4 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 

./RAID0-test 7 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 

./RAID0-test 32 RAID0_disk1.img RAID0_disk2.img RAID0_disk3.img RAID0_disk4.img 