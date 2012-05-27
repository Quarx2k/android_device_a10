# This script is included in squisher
# It is the final build step (after OTA package)

# Delete unwanted apps
rm -f $REPACK/ota/system/app/RomManager.apk

# Remove big videos
rm -f $REPACK/ota/system/media/video/*.480p.mp4
rm -f $REPACK/ota/system/lib/hw/*.goldfish.so

# Prebuilt Kernel
rm -f $REPACK/ota/boot.img
cp -f $DEVICE_TOP/boot.img $REPACK/ota/boot.img

cp -f $DEVICE_TOP/updater-script $REPACK/ota/META-INF/com/google/android/updater-script
