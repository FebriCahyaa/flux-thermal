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

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include <FluxThermal.hpp>

/**
 * @brief Inotify-based file watcher for Flux Thermal.
 *
 * Watches CONFIG_FILE and FLUX_SYNTHESIS_CORE_FILE (Flux status) for
 * IN_CLOSE_WRITE events and invokes the appropriate callback so the
 * daemon can react immediately without polling.
 */
class InotifyHandler {
public:
    using Callback = std::function<void()>;

    /**
     * @brief Start the inotify watch thread.
     *
     * @param on_config_changed  Called when CONFIG_FILE is written.
     * @param on_flux_status     Called when FLUX_SYNTHESIS_CORE_FILE is written.
     * @param on_wake            Generic wake-up callback (any watched event).
     */
    void start(
        Callback on_config_changed,
        Callback on_flux_status,
        Callback on_wake
    );

    /**
     * @brief Signal the watch thread to stop and join it.
     */
    void stop();

    ~InotifyHandler() { stop(); }

private:
    std::thread thread_;
    std::atomic<bool> stop_flag_{false};
    int inotify_fd_  = -1;
    int config_wd_   = -1;
    int flux_wd_     = -1;

    void watch_loop(Callback on_config, Callback on_flux, Callback on_wake);
};
