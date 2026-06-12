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

#include "FluxThermalCLI.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include <FluxThermal.hpp>
#include <FluxThermalLog.hpp>

#include "FluxThermalConfigStore.hpp"
#include "ThermalProfiler.hpp"
#include "base/ThermalZone/ThermalZoneManager.hpp"

namespace FluxThermalCLI {

static void print_usage(const char *prog) {
    printf(
        "Usage: %s <command>\n"
        "\n"
        "Commands:\n"
        "  daemon            Start the thermal daemon (blocking)\n"
        "  status            Print current thermal status to stdout\n"
        "  scan              Scan and list all thermal zones\n"
        "  set_profile <N>   Force a thermal profile (0=Normal … 4=Critical)\n"
        "\n",
        prog
    );
}

static void cmd_status() {
    // Read current profile
    std::ifstream pf(PROFILE_FILE);
    int profile = -1;
    if (pf.is_open()) pf >> profile;

    const char *profile_names[] = {
        "NORMAL", "COOL", "WARM", "HOT", "CRITICAL"
    };

    printf("Flux Thermal — Status\n");
    printf("  Current profile : %s (%d)\n",
           (profile >= 0 && profile <= 4) ? profile_names[profile] : "UNKNOWN",
           profile);

    // Read thermal_status.json if available
    std::ifstream sf(THERMAL_STATUS_FILE);
    if (sf.is_open()) {
        printf("  Thermal zones   : see %s\n", THERMAL_STATUS_FILE);
    } else {
        printf("  Thermal zones   : status file not yet written\n");
    }

    // Quick live scan
    thermal_zone_mgr.scan();
    printf("  Live max temp   : %.1f °C\n",
           thermal_zone_mgr.max_temp_mc() / 1000.0f);
}

static void cmd_scan() {
    printf("Scanning thermal zones...\n");
    int n = thermal_zone_mgr.scan();
    if (n < 0) {
        fprintf(stderr, "Failed to scan thermal zones\n");
        return;
    }
    printf("%-4s  %-30s  %8s  %s\n", "ID", "Type", "Temp(°C)", "Policy");
    printf("%s\n", std::string(60, '-').c_str());
    for (const auto &z : thermal_zone_mgr.zones()) {
        printf("%-4d  %-30s  %8.1f  %s\n",
               z.zone_id,
               z.type.c_str(),
               z.temp_mc / 1000.0f,
               z.policy.c_str());
    }
}

static void cmd_set_profile(const char *arg) {
    int p = atoi(arg);
    if (p < THERMAL_NORMAL || p > THERMAL_CRITICAL) {
        fprintf(stderr, "Profile must be 0–4\n");
        return;
    }
    ThermalProfiler::apply(static_cast<ThermalProfile>(p));
    printf("Profile set to %d\n", p);
}

int dispatch(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string cmd = argv[1];

    if (cmd == "daemon") {
        // Handled by main()
        return 0;
    }
    if (cmd == "status") {
        cmd_status();
        return EXIT_SUCCESS;
    }
    if (cmd == "scan") {
        cmd_scan();
        return EXIT_SUCCESS;
    }
    if (cmd == "set_profile") {
        if (argc < 3) {
            fprintf(stderr, "set_profile requires a profile number\n");
            return EXIT_FAILURE;
        }
        cmd_set_profile(argv[2]);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd.c_str());
    print_usage(argv[0]);
    return EXIT_FAILURE;
}

} // namespace FluxThermalCLI
