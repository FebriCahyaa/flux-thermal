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

#include <mutex>
#include <string>

#include <rapidjson/document.h>

#include <FluxThermal.hpp>
#include <FluxThermalLog.hpp>

/**
 * @brief Thread-safe singleton that owns the Flux Thermal config.json.
 *
 * Config lives at CONFIG_FILE (/data/adb/.config/flux_thermal/config.json).
 * The InotifyHandler watches CONFIG_FILE and calls reload() on IN_CLOSE_WRITE
 * so changes from the WebUI take effect within one poll cycle.
 */
class FluxThermalConfigStore {
public:
    // ── Configuration structures ─────────────────────────────────────────────

    struct Preferences {
        bool  enabled             = true;  ///< Master kill-switch (keeps module installed)
        bool  aggressive_mode     = false; ///< More aggressive thermal limits at lower thresholds
        bool  protect_battery     = true;  ///< Additional limits when charging
        bool  cool_down_on_charge = true;  ///< Force COOL profile while charging
        int   log_level           = 4;     ///< 0=off … 6=trace

        // Threshold overrides — 0 means use the compiled-in defaults
        int   threshold_cool_mc      = 0;
        int   threshold_warm_mc      = 0;
        int   threshold_hot_mc       = 0;
        int   threshold_critical_mc  = 0;
    };

    struct ZoneFilter {
        /// Zone type substrings that are included in max-temp calculation.
        /// Empty = all zones.  Example: ["cpu", "gpu", "soc", "skin"]
        std::vector<std::string> include_types;
    };

    struct ConfigData {
        Preferences  preferences;
        ZoneFilter   zone_filter;
    };

    // ── Singleton ────────────────────────────────────────────────────────────

    static FluxThermalConfigStore &get_instance() {
        static FluxThermalConfigStore inst;
        return inst;
    }

    // ── Load / Save ──────────────────────────────────────────────────────────

    bool load_config(const std::string &path = CONFIG_FILE);
    bool save_config(const std::string &path = CONFIG_FILE);
    bool reload();

    // ── Accessors ────────────────────────────────────────────────────────────

    [[nodiscard]] Preferences  get_preferences()  const;
    [[nodiscard]] ZoneFilter   get_zone_filter()  const;
    [[nodiscard]] ConfigData   get_config()       const;

    void set_preferences(const Preferences &p);

    // ── Effective threshold helpers ──────────────────────────────────────────

    [[nodiscard]] int effective_threshold_cool()     const;
    [[nodiscard]] int effective_threshold_warm()     const;
    [[nodiscard]] int effective_threshold_hot()      const;
    [[nodiscard]] int effective_threshold_critical() const;

private:
    FluxThermalConfigStore()  = default;
    ~FluxThermalConfigStore() = default;
    FluxThermalConfigStore(const FluxThermalConfigStore &) = delete;
    FluxThermalConfigStore &operator=(const FluxThermalConfigStore &) = delete;

    bool create_default_config();
    bool parse_config(const rapidjson::Document &doc);

    mutable std::mutex mutex_;
    ConfigData config_;
    std::string config_path_ = CONFIG_FILE;
};

#define thermal_config_store FluxThermalConfigStore::get_instance()
