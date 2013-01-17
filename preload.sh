#!/system/bin/sh

# Preload system APKs
find /system/app/ -type f -exec cat {} \; > /dev/null

# Preload <25M user APKs
find /data/app/ -type f -size -$((25*1024))k -exec cat {} \; > /dev/null
