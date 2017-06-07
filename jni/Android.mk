LOCAL_PATH := $(call my-dir)/..
include $(CLEAR_VARS)

LOCAL_MODULE := neon
LOCAL_SRC_FILES := neon.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS := -pie -fPIE -v -O3 -ftree-vectorize -mfloat-abi=softfp -mfpu=neon
# LOCAL_CFLAGS := -O0 -ggdb3 -fno-inline -g -ftree-vectorize -mfloat-abi=softfp -mfpu=neon
# LOCAL_LDLIBS := -lrt
# LOCAL_LDFLAGS := -lrt
LOCAL_LDFLAGS := -pie -fPIE

TARGET_PLATFORM := android-20
TARGET_ARCH_ABI := armeabi-v7a
# LOCAL_ARM_MODE := arm

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)    
	LOCAL_ARM_NEON := true    
endif


# include $(BUILD_STATIC_LIBRARY)
include $(BUILD_EXECUTABLE)
####################
include $(CLEAR_VARS)

LOCAL_MODULE := serial
LOCAL_SRC_FILES := serial.c
LOCAL_CFLAGS := -pie -fPIE -v
LOCAL_LDFLAGS := -pie -fPIE

include $(BUILD_EXECUTABLE)
####################
include $(CLEAR_VARS)

LOCAL_MODULE := gen_testcase
LOCAL_SRC_FILES := gen_testcase.c

include $(BUILD_EXECUTABLE)
####################
include $(CLEAR_VARS)

LOCAL_MODULE := check-result
LOCAL_SRC_FILES := check-result.c

include $(BUILD_EXECUTABLE)
####################
