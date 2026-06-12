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

#include "ThermalProfiler.hpp"

#include <cstdlib>
#include <fstream>
#include <string>

#include <FluxThermalLog.hpp>
#include <Write2File.hpp>

#include "FluxThermalConfigStore.hpp"
#include "base/ThermalZone/ThermalZoneManager.hpp"

// ── Helpers ───────────────────────────────────────────────────────────────────

static SoCType read_soc() {
    std::ifstream f(FLUX_SOC_FILE);
    if (!f.is_open()) return SOC_UNKNOWN;
    int v = 0;
    f >> v;
    return static_cast<SoCType>(v);
}

static bool read_is_gki() {
    std::ifstream f(FLUX_IS_GKI_FILE);
    if (!f.is_open()) return false;
    int v = 0;
    f >> v;
    return v != 0;
}

static const char *profile_name(ThermalProfile p) {
    switch (p) {
        case THERMAL_NORMAL:   return "normal";
        case THERMAL_COOL:     return "cool";
        case THERMAL_WARM:     return "warm";
        case THERMAL_HOT:      return "hot";
        case THERMAL_CRITICAL: return "critical";
    }
    return "normal";
}

// ── Full-param apply ──────────────────────────────────────────────────────────

void ThermalProfiler::apply(
    ThermalProfile profile,
    SoCType        soc,
    bool           charging,
    bool           aggressive,
    int            max_temp_mc,
    bool           is_gki)
{
    LOGI_TAG("Profiler", "Applying thermal profile: {} (soc={} charging={} aggressive={} temp={}mc gki={})",
             profile_name(profile), static_cast<int>(soc),
             charging, aggressive, max_temp_mc, is_gki);

    // Persist current profile for WebUI / shell queries
    write2file(PROFILE_FILE, static_cast<int>(profile), "\n");

    // Build environment for the shell script
    setenv("FLUX_THERMAL_SOC",         std::to_string(static_cast<int>(soc)).c_str(), 1);
    setenv("FLUX_THERMAL_IS_GKI",      is_gki      ? "1" : "0",  1);
    setenv("FLUX_THERMAL_CHARGING",    charging    ? "1" : "0",  1);
    setenv("FLUX_THERMAL_AGGRESSIVE",  aggressive  ? "1" : "0",  1);
    setenv("FLUX_THERMAL_MAX_TEMP_MC", std::to_string(max_temp_mc).c_str(), 1);

    // Invoke the per-SoC profiler shell script
    const std::string cmd = std::string("flux_thermal_profiler ") + profile_name(profile);
    int rc = system(cmd.c_str());
    if (rc != 0)
        LOGE_TAG("Profiler", "flux_thermal_profiler returned {}", rc);

    // Update the live thermal_status.json after applying
    thermal_zone_mgr.dump_to_status_file();
}

// ── Convenience overload (reads state from disk + cache) ─────────────────────

void ThermalProfiler::apply(ThermalProfile profile) {
    const auto prefs = thermal_config_store.get_preferences();

    // Read Flux-provided state
    SoCType soc   = read_soc();
    bool is_gki   = read_is_gki();
    int  max_temp = thermal_zone_mgr.max_temp_mc();

    // Read charging state from Flux synthesis_core.json if available
    // (simple text parse — avoids pulling in SynthesisCore headers)
    bool charging = false;
    {
        FILE *fp = fopen(FLUX_SYNTHESIS_CORE_FILE, "r");
        if (fp) {
            char line[128];
            while (fgets(line, sizeof(line), fp)) {
                int v = 0;
                if (sscanf(line, "charging_state %d", &v) == 1) {
                    charging = (v != 0);
                    break;
                }
            }
            fclose(fp);
        }
    }

    apply(profile, soc, charging, prefs.aggressive_mode, max_temp, is_gki);
}
