# Copyright (C) 2010 The Android Open Source Project
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
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CXX := $(CXX) 
LOCAL_CFLAGS        := -Werror -DKTX_OPENGL_ES2=1 -DSUPPORT_SOFTWARE_ETC_UNPACK=0 -g -DNDK_DEBUG=1
#LOCAL_C_INCLUDES    := $(LOCAL_PATH)/stb $(LOCAL_PATH)/libktx
LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := main.cpp \
                   file.cpp \
				   Framebuffer.cpp \
				   Scene.cpp \
				   GLUtils.cpp \
#                  texture.cpp \
#   				   framebuffer.cpp \
#   			   stb/stb_image.cpp \
#				   libktx/checkheader.c \
#				   libktx/etcunpack.c \
#				   libktx/hashtable.c \
#				   libktx/loader.c \
#				   libktx/swap.c \
#				   libktx/writer.c \
				   
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
