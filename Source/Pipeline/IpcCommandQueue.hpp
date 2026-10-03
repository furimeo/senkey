// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <memory>
#include <functional>

namespace senkey {

struct IpcCommandRequest {
    std::string command;
    std::shared_ptr<std::promise<std::string>> promise;
};

class IpcCommandQueue {
private:
    std::mutex mtx_;
    std::vector<IpcCommandRequest> queue_;
    std::function<void()> wake_cb_;

public:
    void set_wake_callback(std::function<void()> cb) {
        std::lock_guard<std::mutex> lock(mtx_);
        wake_cb_ = std::move(cb);
    }

    void push(IpcCommandRequest req) {
        std::function<void()> cb;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push_back(std::move(req));
            cb = wake_cb_;
        }
        if (cb) {
            cb();
        }
    }

    std::vector<IpcCommandRequest> drain() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<IpcCommandRequest> res;
        res.swap(queue_);
        return res;
    }
};

} // namespace senkey
