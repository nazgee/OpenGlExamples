LOCAL_PATH := $(call my-dir)

# Shared version of the library is named 'glutils'
LOCAL_MODULE := glutils
LOCAL_MODULE_FILENAME := glutils
LOCAL_SRC_FILES := \
  file.cpp \
  GLUtils.cpp \
  Framebuffer.cpp \
  
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2
  
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

