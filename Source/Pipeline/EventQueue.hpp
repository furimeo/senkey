// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <linux/input.h>
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <utility>
#include "Keymap.hpp"

namespace senkey {

enum class ActionType {
    PASSTHROUGH,
    REPLACEMENT
};

struct OutputAction {
    ActionType type = ActionType::PASSTHROUGH;
    struct input_event raw_event{};
    int backs = 0;
    std::string replacement;
    ModifierState mod{};
};

class EventQueue {
private:
    std::deque<OutputAction> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};

public:
    void push(OutputAction action) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(action));
        }
        cv_.notify_one();
    }

    bool pop(OutputAction& action) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return !queue_.empty() || !running_.load();
        });

        if (!running_.load() && queue_.empty()) {
            return false;
        }

        action = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void stop() {
        running_.store(false);
        cv_.notify_all();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }
};

} // namespace senkey
