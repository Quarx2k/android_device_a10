# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

device_path = device/allwinner/a10

DEVICE_PREBUILT := ${device_path}/prebuilt

# Ramdisk
PRODUCT_COPY_FILES += \
	device/allwinner/a10/ramdisk/init.sun4i.rc:root/init.sun4i.rc \
	device/allwinner/a10/ramdisk/init.sun4i.usb.rc:root/init.sun4i.usb.rc \
	device/allwinner/a10/ramdisk/init.trace.rc:root/init.trace.rc \
	device/allwinner/a10/ramdisk/ueventd.rc:root/ueventd.rc \
	device/allwinner/a10/ramdisk/ueventd.sun4i.rc:root/ueventd.sun4i.rc \
	device/allwinner/a10/ramdisk/adbd:root/sbin/adbd \

# EGL Stuff
PRODUCT_COPY_FILES += \
	$(DEVICE_PREBUILT)/lib/egl/libEGL_mali.so:system/lib/egl/libEGL_mali.so \
	$(DEVICE_PREBUILT)/lib/egl/libGLESv1_CM_mali.so:system/lib/egl/libGLESv1_CM_mali.so \
	$(DEVICE_PREBUILT)/lib/egl/libGLESv2_mali.so:system/lib/egl/libGLESv2_mali.so \
	$(DEVICE_PREBUILT)/lib/libMali.so:system/lib/libMali.so \
	$(DEVICE_PREBUILT)/lib/libUMP.so:system/lib/libUMP.so \
	$(DEVICE_PREBUILT)/lib/libMali.so:obj/lib/libMali.so \
	$(DEVICE_PREBUILT)/lib/libUMP.so:obj/lib/libUMP.so \

# Camera
PRODUCT_COPY_FILES += \
	$(DEVICE_PREBUILT)/etc/camera.cfg:system/etc/camera.cfg \
        $(DEVICE_PREBUILT)/etc/media_profiles.xml:system/etc/media_profiles.xml 

# will be removed in future
PRODUCT_COPY_FILES += \
	$(DEVICE_PREBUILT)/lib/hw/gps.sun4i.so:system/lib/hw/gps.sun4i.so \

# OTG/3G stuff
PRODUCT_COPY_FILES += \
        $(DEVICE_PREBUILT)/bin/u3gmonitor:system/bin/u3gmonitor \
        $(DEVICE_PREBUILT)/etc/3g_dongle.cfg:system/etc/3g_dongle.cfg \
        $(DEVICE_PREBUILT)/bin/usb_modeswitch:system/bin/usb_modeswitch \
        $(DEVICE_PREBUILT)/etc/usb_modeswitch.sh:system/etc/usb_modeswitch.sh \
        $(DEVICE_PREBUILT)/bin/chat:system/bin/chat \
        $(DEVICE_PREBUILT)/etc/ppp/call-pppd:system/etc/ppp/call-pppd \
        $(DEVICE_PREBUILT)/etc/ppp/ip-down:system/etc/ppp/ip-down \
        $(DEVICE_PREBUILT)/etc/ppp/ip-up:system/etc/ppp/ip-up 

PRODUCT_COPY_FILES += $(shell test -d $(DEVICE_PREBUILT)/etc/usb_modeswitch.d && \
	find $(DEVICE_PREBUILT)/etc/usb_modeswitch.d -name '*' \
	-printf '%p:system/etc/usb_modeswitch.d/%f ')

# Other stuff
PRODUCT_COPY_FILES += \
	$(DEVICE_PREBUILT)/usr/keylayout/hv_keypad.kl:system/usr/keylayout/hv_keypad.kl \
	$(DEVICE_PREBUILT)/usr/keylayout/axp20-supplyer.kl:system/usr/keylayout/axp20-supplyer.kl \
	$(DEVICE_PREBUILT)/usr/keylayout/sun4i-keyboard.kl:system/usr/keylayout/sun4i-keyboard.kl \
	$(DEVICE_PREBUILT)/usr/idc/ft5x_ts.idc:system/usr/idc/ft5x_ts.idc \
	$(DEVICE_PREBUILT)/usr/idc/gt80x.idc:system/usr/idc/gt80x.idc \
	$(DEVICE_PREBUILT)/usr/idc/qwerty.idc:system/usr/idc/qwerty.idc \
	$(DEVICE_PREBUILT)/usr/idc/qwerty2.idc:system/usr/idc/qwerty2.idc \
	$(DEVICE_PREBUILT)/vendor/firmware/ft5206-1024X600.bin:system/vendor/firmware/ft5206-1024X600.bin \
	$(DEVICE_PREBUILT)/vendor/firmware/ft5206-sc3013-1024X600.bin:system/vendor/firmware/ft5206-sc3013-1024X600.bin \
	$(DEVICE_PREBUILT)/vendor/firmware/ft5206-sc3017-1024X600.bin:system/vendor/firmware/ft5206-sc3017-1024X600.bin \
	$(DEVICE_PREBUILT)/etc/vold.fstab:system/etc/vold.fstab \
	$(DEVICE_PREBUILT)/etc/gps.conf:system/etc/gps.conf \
	$(DEVICE_PREBUILT)/etc/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
	$(DEVICE_PREBUILT)/lib/liballwinner-ril.so:system/lib/liballwinner-ril.so \
	$(DEVICE_PREBUILT)/bin/ntfs-3g:system/bin/ntfs-3g \
	$(DEVICE_PREBUILT)/bin/ntfs-3g.probe:system/bin/ntfs-3g.probe \
	$(DEVICE_PREBUILT)/bin/mkfs.exfat:system/bin/mkfs.exfat \
	$(DEVICE_PREBUILT)/bin/mount.exfat:system/bin/mount.exfat \
 	$(DEVICE_PREBUILT)/bin/fsck.exfat:system/bin/fsck.exfat \
	$(DEVICE_PREBUILT)/etc/media_codecs.xml:system/etc/media_codecs.xml \
	${device_path}/audio/audio_policy.conf:system/etc/audio_policy.conf \


# New CM9 backup list system (addon.d)
PRODUCT_COPY_FILES += \
	${device_path}/releasetools/addon.d/70-gapps.sh:system/addon.d/70-gapps.sh \

#end of a10-blobs.mk
