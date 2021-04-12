#!/bin/bash

USBPATH="/media/usbdisk"
#USBPATH="/mnt/usb"
X=0

while [ $X -eq 0 ]
do
    X=`cat /proc/scsi/scsi | grep PSP | wc -l`
    echo "Trying to mount"
done

psp-strip openttd.elf -o openttd_strip.elf
mount $USBPATH
#cp openttd_strip.elf /mnt/usb/psp/game150/__SCE__NOTTD/eboot.pbp
cp openttd.elf ${USBPATH}/psp/game150/__SCE__OTTD/eboot.pbp
umount $USBPATH
