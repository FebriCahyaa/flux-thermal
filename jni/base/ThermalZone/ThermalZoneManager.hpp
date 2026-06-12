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

#include <functional>
#include <string>
#include <vector>

#include <FluxThermal.hpp>

/**
 * @brief Provides read/write access to /sys/class/thermal/thermal_zone* nodes.
 *
 * Supports any SoC that exposes standard Linux thermal sysfs.  Per-SoC
 * specialisations (e.g. MediaTek's /proc/ppm, Snapdragon's DCVS) are handled
 * by the profiler shell script, not here.
 */
class ThermalZoneManager {
public:
    /**
     * @brief Scan all thermal zones present in sysfs.
     *
     * Iterates /sys/class/thermal/thermal_zone* and populates the internal
     * cache.  Safe to call at any time; clears the previous snapshot.
     *
     * @return Number of zones found, or -1 on I/O error.
     */
    int scan();

    /**
     * @brief Return the last snapshot collected by scan().
     */
    [[nodiscard]] const std::vector<ThermalZoneSnapshot> &zones() const;

    /**
     * @brief Return the highest temperature (millidegrees C) across all zones.
     *
     * Returns INT_MIN if no zones have been scanned yet.
     */
    [[nodiscard]] int max_temp_mc() const;

    /**
     * @brief Return the highest temperature that matches any of the given
     *        type substrings (case-insensitive).
     *
     * Useful for focusing on CPU/GPU zones and ignoring ambient sensors.
     *
     * @param types  List of substrings, e.g. {"cpu", "gpu", "soc"}.
     * @return Highest matched temperature in millidegrees C, or INT_MIN if none.
     */
    [[nodiscard]] int max_temp_for_types(const std::vector<std::string> &types) const;

    /**
     * @brief Set the thermal policy for all zones that currently expose it.
     *
     * Common values: "step_wise", "power_allocator", "user_space".
     * Zones that do not have a writable policy node are silently skipped.
     *
     * @param policy  Policy string to write.
     * @return Number of zones that were successfully updated.
     */
    int set_policy_all(const std::string &policy);

    /**
     * @brief Invoke @p fn for every zone whose type matches any entry in @p types.
     */
    void for_each_matching(
        const std::vector<std::string> &types,
        const std::function<void(const ThermalZoneSnapshot &)> &fn
    ) const;

    /**
     * @brief Write a value to a per-zone trip_point temperature file.
     *
     * trip_point_N_temp is the sysfs file that controls when a trip triggers.
     *
     * @param zone_id    Zone index.
     * @param trip_index Trip point index (0-based).
     * @param temp_mc    Temperature in millidegrees C to write.
     * @return true on success.
     */
    static bool set_trip_point(int zone_id, int trip_index, int temp_mc);

    /**
     * @brief Serialise the current snapshot to JSON and write it to
     *        THERMAL_STATUS_FILE so the WebUI can display live sensor data.
     */
    void dump_to_status_file() const;

    /** Singleton accessor. */
    static ThermalZoneManager &instance() {
        static ThermalZoneManager inst;
        return inst;
    }

    ThermalZoneManager(const ThermalZoneManager &) = delete;
    ThermalZoneManager &operator=(const ThermalZoneManager &) = delete;

private:
    ThermalZoneManager() = default;

    std::vector<ThermalZoneSnapshot> zones_;

    /** Helper: read a single-line integer file; returns 0 on failure. */
    static int read_int_file(const std::string &path);

    /** Helper: read a single-line string file; returns "" on failure. */
    static std::string read_str_file(const std::string &path);

    /** Helper: write a string to a file; returns true on success. */
    static bool write_str_file(const std::string &path, const std::string &val);
};

#define thermal_zone_mgr ThermalZoneManager::instance()
