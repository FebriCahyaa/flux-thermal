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

#include <FluxThermal.hpp>

/**
 * @brief Applies a ThermalProfile by invoking the flux_thermal_profiler script.
 *
 * The daemon calls ThermalProfiler::apply() every time the profile needs to
 * change.  The heavy sysfs work lives in the shell script so it stays editable
 * at runtime without recompiling the daemon.
 *
 * Environment variables set before exec:
 *   FLUX_THERMAL_SOC            — SoC code (0-7)
 *   FLUX_THERMAL_IS_GKI         — 1 if GKI kernel
 *   FLUX_THERMAL_CHARGING       — 1 if charging
 *   FLUX_THERMAL_AGGRESSIVE     — 1 if aggressive mode enabled
 *   FLUX_THERMAL_MAX_TEMP_MC    — Current max zone temperature (millidegrees)
 */
namespace ThermalProfiler {

/**
 * @brief Apply the given thermal profile with explicit runtime parameters.
 * @param profile Target profile.
 * @param soc     SoC type code (read from FLUX_SOC_FILE).
 * @param charging True when the device is charging.
 * @param aggressive True when aggressive mode is on.
 * @param max_temp_mc Current maximum zone temperature.
 * @param is_gki True when running on a GKI kernel.
 */
void apply(
    ThermalProfile profile,
    SoCType        soc,
    bool           charging,
    bool           aggressive,
    int            max_temp_mc,
    bool           is_gki
);

/**
 * @brief Convenience overload that reads all runtime params from cached state.
 * Used by the daemon main loop.
 */
void apply(ThermalProfile profile);

} // namespace ThermalProfiler
