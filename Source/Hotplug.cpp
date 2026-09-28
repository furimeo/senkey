// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Hotplug.hpp"
#include "Logger.hpp"
#include <unistd.h>
#include <poll.h>
#include <chrono>

namespace senkey {

HotplugWatcher::~HotplugWatcher() {
    stop();
}

bool HotplugWatcher::start(std::function<void()> callback) {
    stop();
    on_device_change = callback;

    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        return false;
    }

    watch_fd = inotify_add_watch(inotify_fd, "/dev/input", IN_CREATE | IN_DELETE);
    if (watch_fd < 0) {
        close(inotify_fd);
        inotify_fd = -1;
        return false;
    }

    running = true;
    worker_thread = std::thread(&HotplugWatcher::thread_loop, this);
    return true;
}

void HotplugWatcher::stop() {
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    if (watch_fd >= 0) {
        inotify_rm_watch(inotify_fd, watch_fd);
        watch_fd = -1;
    }
    if (inotify_fd >= 0) {
        close(inotify_fd);
        inotify_fd = -1;
    }
}

void HotplugWatcher::thread_loop() {
    char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    struct pollfd pfd;
    pfd.fd = inotify_fd;
    pfd.events = POLLIN;

    while (running) {
        int ret = poll(&pfd, 1, 500);
        if (ret <= 0) continue;

        ssize_t len = read(inotify_fd, buffer, sizeof(buffer));
        if (len > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Logger::info("Device change in /dev/input, re-scanning keyboards...");
            if (on_device_change) {
                on_device_change();
            }
        }
    }
}

} // namespace senkey
