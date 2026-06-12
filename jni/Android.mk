LOCAL_PATH := $(call my-dir)

# ── External deps ──────────────────────────────────────────────────────────────
include $(LOCAL_PATH)/external/Android.mk

# ── Base libraries ─────────────────────────────────────────────────────────────
include $(LOCAL_PATH)/base/InotifyWatcher/Android.mk
include $(LOCAL_PATH)/base/LockFile/Android.mk
include $(LOCAL_PATH)/base/ThermalZone/Android.mk

# ── Main daemon ────────────────────────────────────────────────────────────────
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
    spdlog

LOCAL_CPPFLAGS  := -std=c++17 -O2 -Wall -Wextra -fexceptions -fPIE
LOCAL_LDFLAGS   := -pie -static-libstdc++

include $(BUILD_EXECUTABLE)
