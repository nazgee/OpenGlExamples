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

LOCAL_PATH:= $(call my-dir)

##################################################################
#
# NOTE: This is a helper to build this sample code using the
# Android platform build system, inside of its source tree.  This
# is NOT part of the NDK and is not for use with the NDK build
# system.
#
##################################################################

# ----------------------------------------------------------------
# Packaging .ap (and Java code if there was some)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := samples

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := NativeActivity

LOCAL_CERTIFICATE := shared

LOCAL_JNI_SHARED_LIBRARIES := libnative-activity

LOCAL_SDK_VERSION := current

include $(BUILD_PACKAGE)
