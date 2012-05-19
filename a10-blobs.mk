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
	device/allwinner/a10/prebuilt/lib/libMali.so:system/lib/libMali.so \
	device/allwinner/a10/prebuilt/lib/libUMP.so:system/lib/libUMP.so \
	device/allwinner/a10/prebuilt/lib/libMali.so:obj/lib/libMali.so \
	device/allwinner/a10/prebuilt/lib/libUMP.so:obj/lib/libUMP.so \

# Camera
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/lib/hw/camera.a10.so:system/lib/hw/camera.exDroid.so \
	device/allwinner/a10/prebuilt/etc/camera.cfg:system/etc/camera.cfg \
        device/allwinner/a10/prebuilt/etc/media_profiles.xml:system/etc/media_profiles.xml 

# will be removed in future
PRODUCT_COPY_FILES += \
	device/allwinner/a10/prebuilt/lib/hw/gps.a10.so:system/lib/hw/gps.exDroid.so \

# OTG/3G stuff
PRODUCT_COPY_FILES += \
        device/allwinner/a10/prebuilt/bin/u3gmonitor:system/bin/u3gmonitor \
        device/allwinner/a10/prebuilt/etc/3g_dongle.cfg:system/etc/3g_dongle.cfg \
        device/allwinner/a10/prebuilt/bin/usb_modeswitch:system/bin/usb_modeswitch \
        device/allwinner/a10/prebuilt/etc/usb_modeswitch.sh:system/etc/usb_modeswitch.sh \

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
	device/allwinner/a10/prebuilt/lib/liballwinner-ril.so:system/lib/liballwinner-ril.so \
	device/allwinner/a10/prebuilt/bin/erase_misc.sh:system/bin/erase_misc.sh \
        device/allwinner/a10/prebuilt/bootanimation.zip:system/media/bootanimation.zip \


#end of a10-blobs.mk
