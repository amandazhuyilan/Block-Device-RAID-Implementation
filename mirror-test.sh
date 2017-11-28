#!/bin/sh

# Compile mirror-test.c. use -v -da -Q for debugging in gdb

gcc -g -w -o mirror-test mirror-test.c image.c homework.c -g


# Amanda: Create 5120-byte file containing different characters for disk images.

dd if=/dev/zero bs=512 count=10 | tr '\0' '1' > mirror_disk1.img

dd if=/dev/zero bs=512 count=10 | tr '\0' '2' > mirror_disk2.img

dd if=/dev/zero bs=512 count=10 | tr '\0' 'R' > mirror_new_disk.img

./mirror-test mirror_disk1.img mirror_disk2.img