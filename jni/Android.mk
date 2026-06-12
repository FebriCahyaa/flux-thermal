JNI_ROOT_PATH := $(call my-dir)
LOCAL_PATH := $(JNI_ROOT_PATH)

# ── External deps ──────────────────────────────────────────────────────────────
include $(JNI_ROOT_PATH)/external/Android.mk

# ── Base libraries ─────────────────────────────────────────────────────────────
include $(JNI_ROOT_PATH)/base/InotifyWatcher/Android.mk
include $(JNI_ROOT_PATH)/base/LockFile/Android.mk
include $(JNI_ROOT_PATH)/base/ThermalZone/Android.mk

# ── Main daemon ────────────────────────────────────────────────────────────────
LOCAL_PATH := $(JNI_ROOT_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    := flux_thermald
LOCAL_SRC_FILES := \
    Main.cpp \
    InotifyHandler.cpp \
    FluxThermalCLI.cpp \
    FluxThermalConfigStore.cpp \
    ThermalProfiler.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/external \
    $(LOCAL_PATH)/base/InotifyWatcher \
    $(LOCAL_PATH)/base/LockFile \
    $(LOCAL_PATH)/base/ThermalZone

LOCAL_STATIC_LIBRARIES := \
    InotifyWatcher \
    LockFile \
    ThermalZoneManager \
    spdlog \
    rapidjson

LOCAL_CPPFLAGS  := -std=c++17 -O2 -Wall -Wextra -fexceptions -fPIE
LOCAL_LDFLAGS   := -pie -static-libstdc++

include $(BUILD_EXECUTABLE)
