# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	rild.c


LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libril

ifeq ($(TARGET_ARCH),arm)
LOCAL_SHARED_LIBRARIES += libdl
endif # arm

LOCAL_CFLAGS := -DRIL_SHLIB
LOCAL_MODULE:= rild_sun4i
LOCAL_MODULE_STEM := rild
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

