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

# EGL Stuff
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/lib/egl/libEGL_mali.so:system/lib/egl/libEGL_mali.so \
	device/allwinner/a10/prebuilt/lib/egl/libGLESv1_CM_mali.so:system/lib/egl/libGLESv1_CM_mali.so \
	device/allwinner/a10/prebuilt/lib/egl/libGLESv2_mali.so:system/lib/egl/libGLESv2_mali.so \
	device/allwinner/a10/prebuilt/lib/hw/display.a10.so:system/lib/hw/display.a10.so \
	device/allwinner/a10/prebuilt/lib/hw/gralloc.exDroid.so:system/lib/hw/gralloc.exDroid.so \
	device/allwinner/a10/prebuilt/lib/hw/hwcomposer.exDroid.so:system/lib/hw/hwcomposer.exDroid.so \
	device/allwinner/a10/prebuilt/lib/libMali.so:system/lib/libMali.so \
	device/allwinner/a10/prebuilt/lib/libUMP.so:system/lib/libUMP.so 

# Camera
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/lib/hw/camera.a10.so:system/lib/egl/hw/camera.a10.so \
	device/allwinner/a10/prebuilt/etc/camera.cfg:system/etc/camera.cfg \
        device/allwinner/a10/prebuilt/etc/media_profiles.xml:system/etc/media_profiles.xml 
# Codecs
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/lib/libCedarA.so:system/lib/libCedarA.so \
	device/allwinner/a10/prebuilt/lib/libcedarv.so:system/lib/libcedarv.so \
	device/allwinner/a10/prebuilt/lib/libCedarX.so:system/lib/libCedarX.so \
	device/allwinner/a10/prebuilt/lib/libcedarxbase.so:system/lib/libcedarxbase.so \
	device/allwinner/a10/prebuilt/lib/libswdrm.so:system/lib/libswdrm.so \
	device/allwinner/a10/prebuilt/lib/libcedarxosal.so:system/lib/libcedarxosal.so 

# will be removed in future
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/bin/wpa_supplicant:system/bin/wpa_supplicant \
	device/allwinner/a10/prebuilt/lib/hw/lights.a10.so:system/lib/hw/lights.a10.so \
	device/allwinner/a10/prebuilt/lib/hw/sensors.a10.so:system/lib/hw/sensors.a10.so \

# OTG/3G stuff
PRODUCT_COPY_FILES += \
        device/allwinner/a10/prebuilt/bin/u3gmonitor:system/bin/u3gmonitor \
        device/allwinner/a10/prebuilt/bin/usb_modeswitch:system/bin/usb_modeswitch \
	device/allwinner/a10/prebuilt/etc/3g_dongle.cfg:system/etc/3g_dongle.cfg \

PRODUCT_COPY_FILES += $(shell test -d device/allwinner/a10/prebuilt/etc/usb_modeswitch.d && \
	find device/allwinner/a10/prebuilt/etc/usb_modeswitch.d -name '*' \
	-printf '%p:system/etc/usb_modeswitch.d/%f ')

# Other stuff
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/usr/keylayout/hv_keypad.kl:system/usr/keylayout/hv_keypad.kl \
	device/allwinner/a10/prebuilt/usr/keylayout/axp20-supplyer.kl:system/usr/keylayout/axp20-supplyer.kl \
	device/allwinner/a10/prebuilt/usr/keylayout/sun4i-keyboard.kl:system/usr/keylayout/sun4i-keyboard.kl \
	device/allwinner/a10/prebuilt/usr/idc/ft5x_ts.idc:system/usr/idc/ft5x_ts.idc \
	device/allwinner/a10/prebuilt/usr/idc/gt80x.idc:system/usr/idc/gt80x.idc \
	device/allwinner/a10/prebuilt/usr/idc/qwerty.idc:system/usr/idc/qwerty.idc \
	device/allwinner/a10/prebuilt/usr/idc/qwerty2.idc:system/usr/idc/qwerty2.idc \
	device/allwinner/a10/prebuilt/vendor/firmware/ft5206-1024X600.bin:system/vendor/firmware/ft5206-1024X600.bin \
	device/allwinner/a10/prebuilt/etc/vold.fstab:system/etc/vold.fstab \
	device/allwinner/a10/prebuilt/etc/gps.conf:system/etc/gps.conf \
	device/allwinner/a10/prebuilt/etc/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
	device/allwinner/a10/prebuilt/lib/liballwinner-ril.so:system/lib/liballwinner-ril.so 

#end of a10-blobs.mk
