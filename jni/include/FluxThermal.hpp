/*
 * Copyright (C) 2024-2026 FebriCahyaa
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

#define NOTIFY_TITLE      "Flux Thermal"
#define LOG_TAG           "FluxThermal"

// ── Dependency paths (Flux must be installed) ─────────────────────────────────
#define FLUX_MODPATH       "/data/adb/modules/flux"
#define FLUX_CONFIG_DIR    "/data/adb/.config/flux"
#define FLUX_SYNTHESIS_CORE_FILE  FLUX_CONFIG_DIR "/synthesis_core.json"
#define FLUX_SOC_FILE      FLUX_CONFIG_DIR "/soc_recognition"
#define FLUX_IS_GKI_FILE   FLUX_CONFIG_DIR "/is_gki"

// ── Flux Thermal own paths ────────────────────────────────────────────────────
#define CONFIG_DIR         "/data/adb/.config/flux_thermal"
#define MODPATH            "/data/adb/modules/flux_thermal"

#define LOCK_FILE          CONFIG_DIR "/.lock"
#define LOG_FILE           CONFIG_DIR "/flux_thermal.log"
#define CONFIG_FILE        CONFIG_DIR "/config.json"
#define PROFILE_FILE       CONFIG_DIR "/current_thermal_profile"
#define THERMAL_STATUS_FILE CONFIG_DIR "/thermal_status.json"
#define MODULE_PROP        MODPATH "/module.prop"

// ── Thermal thresholds ────────────────────────────────────────────────────────

/// Temperature (millidegrees Celsius) at which COOL profile activates
static constexpr int TEMP_THRESHOLD_COOL    = 40000;  // 40°C
/// Temperature at which WARM profile activates
static constexpr int TEMP_THRESHOLD_WARM    = 50000;  // 50°C
/// Temperature at which HOT profile activates
static constexpr int TEMP_THRESHOLD_HOT     = 60000;  // 60°C
/// Temperature at which CRITICAL profile activates (emergency throttle)
static constexpr int TEMP_THRESHOLD_CRITICAL = 70000; // 70°C

/// Hysteresis: temperature must drop this far below threshold before recovering
static constexpr int TEMP_HYSTERESIS        = 3000;   // 3°C

/// Default polling interval (milliseconds) when no inotify events arrive
static constexpr int DEFAULT_POLL_INTERVAL_MS = 3000;

/// How often (ms) to force a full thermal re-read even without inotify event
static constexpr int FORCE_READ_INTERVAL_MS   = 10000;

// ── Thermal profile enum ──────────────────────────────────────────────────────

enum ThermalProfile : int {
    THERMAL_NORMAL    = 0,  ///< Baseline — all limits lifted
    THERMAL_COOL      = 1,  ///< Light load limit
    THERMAL_WARM      = 2,  ///< Moderate throttle
    THERMAL_HOT       = 3,  ///< Aggressive throttle
    THERMAL_CRITICAL  = 4,  ///< Emergency — minimum clocks, all cores limited
};

// ── SoC codes (mirror of Flux) ────────────────────────────────────────────────

enum SoCType : int {
    SOC_UNKNOWN   = 0,
    SOC_MEDIATEK  = 1,
    SOC_SNAPDRAGON= 2,
    SOC_EXYNOS    = 3,
    SOC_UNISOC    = 4,
    SOC_TENSOR    = 5,
    SOC_TEGRA     = 6,
    SOC_KIRIN     = 7,
};

// ── Snapshot of a single thermal zone ─────────────────────────────────────────

struct ThermalZoneSnapshot {
    int   zone_id   = -1;
    std::string type;
    int   temp_mc   = 0;   ///< millidegrees Celsius
    std::string policy;
};
