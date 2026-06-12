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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "FluxThermalCLI.hpp"
#include "FluxThermalConfigStore.hpp"
#include "InotifyHandler.hpp"
#include "ThermalProfiler.hpp"

#include <FluxThermal.hpp>
#include <FluxThermalLog.hpp>
#include <LockFile.hpp>
#include <ModuleProperty.hpp>
#include <ShellUtility.hpp>
#include <SignalHandler.hpp>
#include <Write2File.hpp>

#include "base/ThermalZone/ThermalZoneManager.hpp"

// ── Forward declarations ─────────────────────────────────────────────────────

static void run_daemon();

// ── Signal / wake infrastructure ────────────────────────────────────────────

static int              wake_event_fd = -1;
static std::atomic<bool> stop_requested{false};

static void signal_wake() {
    const int fd = wake_event_fd;
    if (fd >= 0) {
        uint64_t v = 1;
        ssize_t r = write(fd, &v, sizeof(v));
        (void)r;
    }
}

static void signal_stop() {
    stop_requested.store(true, std::memory_order_relaxed);
    signal_wake();
}

// ── Module.prop helpers ──────────────────────────────────────────────────────

static void set_module_status(const std::string &status) {
    const std::string desc = "[" + status + "] Thermal management for Flux Tweaks.";
    try {
        ModuleProperty::Change(MODULE_PROP, {{"description", desc}});
    } catch (...) {}
}

static void notify_fatal(const std::string &msg) {
    const std::string full = "ERROR: " + msg;
    LOGC("{}", full);
    // Best-effort: send an Android toast via Magisk/KernelSU notify utility
    systemv("cmd_notify '%s' 2>/dev/null || true", full.c_str());
    set_module_status("❌ " + msg);
}

// ── Dependency check: require Flux to be installed ───────────────────────────

static bool flux_is_installed() {
    // Flux module directory must exist and not be disabled
    std::ifstream mod(std::string(FLUX_MODPATH) + "/module.prop");
    if (!mod.is_open()) return false;

    // Also check that the Flux config dir exists (daemon has run at least once)
    struct stat st{};
    return stat(FLUX_CONFIG_DIR, &st) == 0;
}

// ── Lock file ────────────────────────────────────────────────────────────────

static LockFile daemon_lock{LOCK_FILE};

static bool acquire_daemon_lock() {
    return daemon_lock.acquire(LockFile::AcquireMode::NonBlocking, LockFile::LockType::Exclusive);
}

// ── Daemon state ──────────────────────────────────────────────────────────────

struct DaemonState {
    ThermalProfile current_profile = THERMAL_NORMAL;
    int            max_temp_mc     = 0;
    bool           charging        = false;

    /// Debounce: time of the last profile change (used for hysteresis)
    std::chrono::steady_clock::time_point last_switch_tp{};

    /// How many consecutive polls saw the same direction of temp change
    int  trend_count = 0;
    bool trend_rising = false;
};

// ── Profile decision logic ───────────────────────────────────────────────────

/**
 * @brief Decide the desired profile from the current temperature and state.
 *
 * Implements hysteresis: once we enter a profile tier, the temperature must
 * drop (threshold - TEMP_HYSTERESIS) before we step back down.
 */
static ThermalProfile decide_profile(const DaemonState &state) {
    const int t = state.max_temp_mc;

    const int t_critical = thermal_config_store.effective_threshold_critical();
    const int t_hot      = thermal_config_store.effective_threshold_hot();
    const int t_warm     = thermal_config_store.effective_threshold_warm();
    const int t_cool     = thermal_config_store.effective_threshold_cool();
    const int hyst       = TEMP_HYSTERESIS;

    // Stepped thresholds with hysteresis going DOWN
    const ThermalProfile cur = state.current_profile;

    if (t >= t_critical) return THERMAL_CRITICAL;

    if (t >= t_hot) {
        if (cur == THERMAL_CRITICAL && t > t_critical - hyst) return THERMAL_CRITICAL;
        return THERMAL_HOT;
    }

    if (t >= t_warm) {
        if (cur == THERMAL_CRITICAL && t > t_critical - hyst) return THERMAL_CRITICAL;
        if (cur == THERMAL_HOT && t > t_hot - hyst) return THERMAL_HOT;
        return THERMAL_WARM;
    }

    if (t >= t_cool) {
        if (cur >= THERMAL_HOT && t > t_hot - hyst) return THERMAL_HOT;
        if (cur == THERMAL_WARM && t > t_warm - hyst) return THERMAL_WARM;
        return THERMAL_COOL;
    }

    // Below all thresholds
    if (cur == THERMAL_COOL && t > t_cool - hyst) return THERMAL_COOL;
    return THERMAL_NORMAL;
}

// ── Daemon loop ───────────────────────────────────────────────────────────────

static void run_daemon() {
    LOGI("Flux Thermal daemon starting");

    // ── Check Flux dependency ────────────────────────────────────────────────
    if (!flux_is_installed()) {
        LOGC("Flux Tweaks is not installed! Flux Thermal requires Flux.");
        notify_fatal("Flux Tweaks not found — install Flux first");
        // Write a sentinel so service.sh can read it
        write2file(CONFIG_DIR "/dependency_error", "flux_missing\n");
        return;
    }
    LOGI("Flux dependency check: OK");

    // ── Acquire singleton lock ───────────────────────────────────────────────
    if (!acquire_daemon_lock()) {
        LOGI("Another flux_thermald instance is running, exiting");
        return;
    }

    // ── Load config ──────────────────────────────────────────────────────────
    thermal_config_store.load_config();

    // ── Initial thermal zone scan ────────────────────────────────────────────
    thermal_zone_mgr.scan();
    LOGI("Found {} thermal zones, max temp: {:.1f}°C",
         thermal_zone_mgr.zones().size(),
         thermal_zone_mgr.max_temp_mc() / 1000.0f);

    // ── Set up wake eventfd ──────────────────────────────────────────────────
    wake_event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wake_event_fd < 0) {
        LOGE("Failed to create eventfd: {}", strerror(errno));
        return;
    }

    // ── Inotify watcher ──────────────────────────────────────────────────────
    InotifyHandler inotify;
    inotify.start(
        /* on_config_changed */ []() {
            LOGI("Config changed — reloading");
            thermal_config_store.reload();
            signal_wake();
        },
        /* on_flux_status */ []() {
            signal_wake(); // Flux updated synthesis_core.json
        },
        /* on_wake */ []() {
            signal_wake();
        }
    );

    // ── Initial profile application ──────────────────────────────────────────
    DaemonState state;
    state.max_temp_mc = thermal_zone_mgr.max_temp_mc();
    state.current_profile = decide_profile(state);
    ThermalProfiler::apply(state.current_profile);
    set_module_status("🌡️ Running");

    // ── Poll / event loop ─────────────────────────────────────────────────────

    using Clock = std::chrono::steady_clock;
    auto last_force_read = Clock::now();

    struct pollfd pfds[1];
    pfds[0].fd     = wake_event_fd;
    pfds[0].events = POLLIN;

    LOGI("Daemon loop started");

    while (!stop_requested.load()) {
        int ret = poll(pfds, 1, DEFAULT_POLL_INTERVAL_MS);

        if (stop_requested.load()) break;

        // Drain eventfd if woken
        if (ret > 0 && (pfds[0].revents & POLLIN)) {
            uint64_t v;
            ssize_t r = read(wake_event_fd, &v, sizeof(v));
            (void)r;
        }

        // ── Check if master enable is off ────────────────────────────────
        if (!thermal_config_store.get_preferences().enabled) {
            LOGD("Master switch off — skipping cycle");
            if (state.current_profile != THERMAL_NORMAL) {
                state.current_profile = THERMAL_NORMAL;
                ThermalProfiler::apply(THERMAL_NORMAL);
            }
            continue;
        }

        // ── Re-scan thermal zones ─────────────────────────────────────────
        auto now = Clock::now();
        bool force = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_force_read).count() >= FORCE_READ_INTERVAL_MS;

        if (force || ret > 0) {
            thermal_zone_mgr.scan();
            if (force) last_force_read = now;
        }

        // ── Read charging state from Flux ─────────────────────────────────
        {
            FILE *fp = fopen(FLUX_SYNTHESIS_CORE_FILE, "r");
            if (fp) {
                char line[128];
                while (fgets(line, sizeof(line), fp)) {
                    int v = 0;
                    if (sscanf(line, "charging_state %d", &v) == 1) {
                        state.charging = (v != 0);
                        break;
                    }
                }
                fclose(fp);
            }
        }

        // If protect_battery is on and we're charging, enforce at least COOL
        auto prefs = thermal_config_store.get_preferences();
        int max_temp = thermal_zone_mgr.max_temp_mc();

        if (state.charging && prefs.cool_down_on_charge && max_temp < TEMP_THRESHOLD_COOL) {
            max_temp = TEMP_THRESHOLD_COOL; // artificially clamp upward
        }

        state.max_temp_mc = max_temp;

        // ── Decide and apply profile ──────────────────────────────────────
        ThermalProfile desired = decide_profile(state);
        if (desired != state.current_profile) {
            LOGI("Profile change: {} → {} (temp={:.1f}°C charging={})",
                 static_cast<int>(state.current_profile),
                 static_cast<int>(desired),
                 max_temp / 1000.0f,
                 state.charging);
            state.current_profile = desired;
            state.last_switch_tp  = now;
            ThermalProfiler::apply(desired);
        } else {
            // Still dump status periodically for WebUI freshness
            if (force) thermal_zone_mgr.dump_to_status_file();
        }
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    LOGI("Daemon shutting down");
    inotify.stop();
    if (wake_event_fd >= 0) {
        close(wake_event_fd);
        wake_event_fd = -1;
    }
    // Restore normal profile on exit
    ThermalProfiler::apply(THERMAL_NORMAL);
    set_module_status("⏹️ Stopped");
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    // If no args or first arg is "daemon", start the daemon
    if (argc < 2 || strcmp(argv[1], "daemon") == 0) {
        // Init logging
        FluxThermalLog::init(LOG_FILE);
        LOGI("flux_thermald v{} starting", "1.0");

        // Register signal handlers (SIGTERM / SIGINT → graceful stop)
        SignalHandler::register_all([](int) { signal_stop(); });

        run_daemon();
        return EXIT_SUCCESS;
    }

    // Otherwise delegate to the CLI dispatcher
    FluxThermalLog::init(LOG_FILE);
    return FluxThermalCLI::dispatch(argc, argv);
}
