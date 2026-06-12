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

#include "InotifyHandler.hpp"

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <poll.h>
#include <sys/inotify.h>

#include <FluxThermalLog.hpp>

void InotifyHandler::start(Callback on_config, Callback on_flux, Callback on_wake) {
    stop_flag_.store(false);
    thread_ = std::thread(&InotifyHandler::watch_loop, this,
                          std::move(on_config), std::move(on_flux), std::move(on_wake));
}

void InotifyHandler::stop() {
    stop_flag_.store(true);
    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
    if (thread_.joinable()) thread_.join();
}

void InotifyHandler::watch_loop(Callback on_config, Callback on_flux, Callback on_wake) {
    inotify_fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify_fd_ < 0) {
        LOGE_TAG("Inotify", "inotify_init1 failed: {}", strerror(errno));
        return;
    }

    // Watch config directory (catches writes to any file inside)
    config_wd_ = inotify_add_watch(inotify_fd_, CONFIG_DIR, IN_CLOSE_WRITE);
    if (config_wd_ < 0)
        LOGW_TAG("Inotify", "Cannot watch {}: {}", CONFIG_DIR, strerror(errno));

    // Watch Flux config directory for synthesis_core.json updates
    flux_wd_ = inotify_add_watch(inotify_fd_, FLUX_CONFIG_DIR, IN_CLOSE_WRITE);
    if (flux_wd_ < 0)
        LOGW_TAG("Inotify", "Cannot watch {}: {}", FLUX_CONFIG_DIR, strerror(errno));

    constexpr size_t BUF_LEN = sizeof(inotify_event) * 32 + 256 * 32;
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(inotify_event))));

    struct pollfd pfd{ .fd = inotify_fd_, .events = POLLIN, .revents = 0 };

    LOGI_TAG("Inotify", "Watch thread started");

    while (!stop_flag_.load()) {
        int ret = poll(&pfd, 1, 2000 /* ms timeout */);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue; // timeout — just loop

        ssize_t len = read(inotify_fd_, buf, BUF_LEN);
        if (len <= 0) {
            if (errno == EAGAIN) continue;
            break;
        }

        bool config_changed = false;
        bool flux_changed   = false;

        const char *ptr = buf;
        while (ptr < buf + len) {
            const auto *ev = reinterpret_cast<const inotify_event *>(ptr);

            if (ev->mask & IN_CLOSE_WRITE) {
                const std::string name = ev->len > 0 ? ev->name : "";

                if (ev->wd == config_wd_ && name == "config.json")
                    config_changed = true;

                if (ev->wd == flux_wd_ && name == "synthesis_core.json")
                    flux_changed = true;
            }

            ptr += sizeof(inotify_event) + ev->len;
        }

        if (config_changed) {
            LOGD_TAG("Inotify", "config.json changed");
            if (on_config) on_config();
        }
        if (flux_changed) {
            LOGD_TAG("Inotify", "synthesis_core.json changed");
            if (on_flux) on_flux();
        }
        if ((config_changed || flux_changed) && on_wake) {
            on_wake();
        }
    }

    LOGI_TAG("Inotify", "Watch thread exiting");
    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
}
