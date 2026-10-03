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
#include "Output/OutputBackend.hpp"

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

    TextTransaction to_transaction() const {
        return { backs, replacement, mod };
    }
};

class EventQueue {
private:
    static constexpr size_t MAX_CAPACITY = 256;
    std::deque<OutputAction> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::atomic<size_t> dropped_count_{0};
    std::atomic<size_t> peak_depth_{0};

public:
    void push(OutputAction action) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_.load()) return;

            if (queue_.size() >= MAX_CAPACITY) {
                // Drop oldest event to guarantee bounded memory and prevent stalls
                queue_.pop_front();
                dropped_count_.fetch_add(1, std::memory_order_relaxed);
            }

            queue_.push_back(std::move(action));
            size_t sz = queue_.size();
            size_t cur_peak = peak_depth_.load(std::memory_order_relaxed);
            while (sz > cur_peak && !peak_depth_.compare_exchange_weak(cur_peak, sz, std::memory_order_relaxed)) {}
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

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    size_t get_dropped_count() const {
        return dropped_count_.load(std::memory_order_relaxed);
    }

    size_t get_peak_depth() const {
        return peak_depth_.load(std::memory_order_relaxed);
    }
};

} // namespace senkey
