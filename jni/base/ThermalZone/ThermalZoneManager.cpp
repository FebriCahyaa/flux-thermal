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

#include "ThermalZoneManager.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>

#include <FluxThermalLog.hpp>

// ── Private helpers ──────────────────────────────────────────────────────────

int ThermalZoneManager::read_int_file(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "r");
    if (!fp) return 0;
    int val = 0;
    fscanf(fp, "%d", &val);
    fclose(fp);
    return val;
}

std::string ThermalZoneManager::read_str_file(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string s;
    std::getline(f, s);
    // trim trailing whitespace / newlines
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

bool ThermalZoneManager::write_str_file(const std::string &path, const std::string &val) {
    FILE *fp = fopen(path.c_str(), "w");
    if (!fp) return false;
    fputs(val.c_str(), fp);
    fclose(fp);
    return true;
}

// ── Public methods ────────────────────────────────────────────────────────────

int ThermalZoneManager::scan() {
    zones_.clear();

    DIR *dir = opendir("/sys/class/thermal");
    if (!dir) {
        LOGE_TAG("ThermalZone", "Cannot open /sys/class/thermal: {}", strerror(errno));
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strncmp(ent->d_name, "thermal_zone", 12) != 0) continue;

        const std::string base = std::string("/sys/class/thermal/") + ent->d_name;

        ThermalZoneSnapshot snap;
        // extract zone id from "thermal_zoneN"
        snap.zone_id  = atoi(ent->d_name + 12);
        snap.type     = read_str_file(base + "/type");
        snap.temp_mc  = read_int_file(base + "/temp");
        snap.policy   = read_str_file(base + "/policy");

        zones_.push_back(snap);
    }
    closedir(dir);

    // Sort by zone_id for deterministic iteration
    std::sort(zones_.begin(), zones_.end(),
              [](const ThermalZoneSnapshot &a, const ThermalZoneSnapshot &b) {
                  return a.zone_id < b.zone_id;
              });

    LOGD_TAG("ThermalZone", "Scanned {} thermal zone(s)", zones_.size());
    return static_cast<int>(zones_.size());
}

const std::vector<ThermalZoneSnapshot> &ThermalZoneManager::zones() const {
    return zones_;
}

int ThermalZoneManager::max_temp_mc() const {
    int max = INT_MIN;
    for (const auto &z : zones_) {
        if (z.temp_mc > max) max = z.temp_mc;
    }
    return max;
}

// Case-insensitive substring match
static bool type_matches(const std::string &zone_type, const std::vector<std::string> &types) {
    std::string lower_type = zone_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
    for (const auto &t : types) {
        std::string lower_t = t;
        std::transform(lower_t.begin(), lower_t.end(), lower_t.begin(), ::tolower);
        if (lower_type.find(lower_t) != std::string::npos) return true;
    }
    return false;
}

int ThermalZoneManager::max_temp_for_types(const std::vector<std::string> &types) const {
    int max = INT_MIN;
    for (const auto &z : zones_) {
        if (type_matches(z.type, types) && z.temp_mc > max)
            max = z.temp_mc;
    }
    return max;
}

int ThermalZoneManager::set_policy_all(const std::string &policy) {
    int count = 0;
    for (const auto &z : zones_) {
        const std::string path =
            "/sys/class/thermal/thermal_zone" + std::to_string(z.zone_id) + "/policy";
        if (write_str_file(path, policy)) {
            count++;
        }
    }
    LOGD_TAG("ThermalZone", "Set policy '{}' on {}/{} zones", policy, count, zones_.size());
    return count;
}

void ThermalZoneManager::for_each_matching(
    const std::vector<std::string> &types,
    const std::function<void(const ThermalZoneSnapshot &)> &fn) const
{
    for (const auto &z : zones_) {
        if (type_matches(z.type, types)) fn(z);
    }
}

bool ThermalZoneManager::set_trip_point(int zone_id, int trip_index, int temp_mc) {
    const std::string path =
        "/sys/class/thermal/thermal_zone" + std::to_string(zone_id) +
        "/trip_point_" + std::to_string(trip_index) + "_temp";
    return write_str_file(path, std::to_string(temp_mc));
}

void ThermalZoneManager::dump_to_status_file() const {
    std::ostringstream ss;
    ss << "{\n  \"zones\": [\n";
    for (size_t i = 0; i < zones_.size(); ++i) {
        const auto &z = zones_[i];
        ss << "    {"
           << "\"id\":" << z.zone_id << ","
           << "\"type\":\"" << z.type << "\","
           << "\"temp_mc\":" << z.temp_mc << ","
           << "\"temp_c\":" << (z.temp_mc / 1000.0f) << ","
           << "\"policy\":\"" << z.policy << "\""
           << "}";
        if (i + 1 < zones_.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"max_temp_mc\":" << max_temp_mc() << ",\n";
    ss << "  \"max_temp_c\":" << (max_temp_mc() / 1000.0f) << "\n";
    ss << "}\n";

    FILE *fp = fopen(THERMAL_STATUS_FILE, "w");
    if (!fp) {
        LOGE_TAG("ThermalZone", "Cannot write thermal status file: {}", strerror(errno));
        return;
    }
    fputs(ss.str().c_str(), fp);
    fclose(fp);
}
