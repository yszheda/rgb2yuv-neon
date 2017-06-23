LOCAL_PATH := $(call my-dir)/..
include $(CLEAR_VARS)

LOCAL_MODULE := rgb2yuv
LOCAL_SRC_FILES := main.cpp yuv420sp.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS := -pie -fPIE -v -O3 -ftree-vectorize -mfloat-abi=softfp -mfpu=neon -std=c++11
LOCAL_LDFLAGS := -pie -fPIE

TARGET_PLATFORM := android-20
TARGET_ARCH_ABI := armeabi-v7a

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON := true
endif

include $(BUILD_EXECUTABLE)
