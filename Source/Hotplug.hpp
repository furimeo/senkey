// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <sys/inotify.h>
#include <functional>
#include <thread>
#include <atomic>

namespace senkey {

class HotplugWatcher {
private:
    int inotify_fd = -1;
    int watch_fd = -1;
    std::thread worker_thread;
    std::atomic<bool> running{false};
    std::function<void()> on_device_change;

    void thread_loop();

public:
    HotplugWatcher() = default;
    ~HotplugWatcher();

    bool start(std::function<void()> callback);
    void stop();
};

} // namespace senkey
