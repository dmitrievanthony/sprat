#!/bin/bash

if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi
FSTYPE=ext4
SIZE=256M
while [[ $# -gt 1 ]]; do
	key="$1"
	case $key in
		-f|--fstype)
			FSTYPE="$2"
			shift
			;;
		-s|--size)
			SIZE="$2"
			shift
			;;
		*)	
			UNKNOWN="$1"
			;;
	esac
	shift
done

if [ "$#" -ne 1 ] || [ ! -z "$UNKNOWN" ]; then
	echo "sprat-img.sh IMAGE_NAME"	
	echo -e "Util script which helps to create base images"
	echo -e "Options:"
	echo -e "\t-f --fstype\tFilesystem type (ext4 by default)"
	echo -e "\t-s --size\tSize of the image (256M by default)"
	exit -1
fi

dd if=/dev/zero of=$1 bs=256M count=1 status=noxfer
mkfs -q -t ext4 $1
loop_device=`losetup -f`
losetup $loop_device $1
tmp_dir=`cat /proc/sys/kernel/random/uuid`
mkdir $tmp_dir
mount $loop_device $tmp_dir
cp -r /bin $tmp_dir/
cp -r /lib $tmp_dir/
cp -r /lib64 $tmp_dir/
mkdir $tmp_dir/proc
mkdir $tmp_dir/sys
mkdir $tmp_dir/dev
umount $tmp_dir
rm -rf $tmp_dir
losetup -d $loop_device
