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
#include <vector>

/**
 * @brief Parses and dispatches CLI commands for flux_thermald.
 *
 * Usage:
 *   flux_thermald daemon            — start the daemon (background loop)
 *   flux_thermald status            — print current thermal profile + temps
 *   flux_thermald scan              — scan and print all thermal zones
 *   flux_thermald set_profile <N>   — manually set profile (0-4), debug only
 */
namespace FluxThermalCLI {

/**
 * @brief Entry point: parse argv and return exit code.
 */
int dispatch(int argc, char **argv);

} // namespace FluxThermalCLI
