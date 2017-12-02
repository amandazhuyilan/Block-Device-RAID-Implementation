#!/bin/sh

if [ "$1" = "-v" ] ; then
    verbose=true
fi

# unique disk names
#for i in 1 2 3 4 5 6 7 8; do
for i in 1 2 3 4 5 6 7; do
    disks="$disks /home/asaranprasad/asaranprasad-hw2/img$i.img"
done

# use 'dd' to create disk images, each with 256 512-byte blocks
for d in $disks; do
    # note that 2<&- suppresses dd info messages
    # dd if=/dev/zero bs=512 count=1024 of=$d 2<&- 
    dd if=/dev/zero bs=1 count=1024 of=$d 2<&- 
done