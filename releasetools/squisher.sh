# This script is included in squisher
# It is the final build step (after OTA package)

# Delete unwanted apps
rm -f $REPACK/ota/system/app/RomManager.apk

# Remove big videos
rm -f $REPACK/ota/system/media/video/*.480p.mp4
rm -f $REPACK/ota/system/lib/hw/*.goldfish.so

cp -f $VENDOR_TOP/app/* $REPACK/ota/system/app/

mkdir -p $REPACK/ota/system/etc/terminfo/x
cp $REPACK/ota/system/etc/terminfo/l/linux $REPACK/ota/system/etc/terminfo/x/xterm

rm -f $REPACK/ota/boot.img
cp -f $DEVICE_TOP/boot.img $REPACK/ota/boot.img

cp -f $DEVICE_TOP/updater-script $REPACK/ota/META-INF/com/google/android/updater-script

# use the static busybox as bootmenu shell, and some static utilities
cp -f $DEVICE_OUT/utilities/busybox $REPACK/ota/system/bootmenu/binary/busybox
cp -f $DEVICE_OUT/utilities/lsof $REPACK/ota/system/bootmenu/binary/lsof
