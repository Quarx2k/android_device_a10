#!/sbin/sh
dd if=/dev/zero count=1 | tr '\000' '\377' > /dev/block/nandf
sync