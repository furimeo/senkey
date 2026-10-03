// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include "Pipeline/EventQueue.hpp"
#include "Input/XkbState.hpp"

void test_event_queue() {
    senkey::EventQueue queue;
    std::vector<std::string> received;

    std::thread worker([&queue, &received]() {
        senkey::OutputAction action;
        while (queue.pop(action)) {
            if (action.type == senkey::ActionType::REPLACEMENT) {
                received.push_back(action.replacement);
            }
        }
    });

    queue.push({senkey::ActionType::REPLACEMENT, {}, 1, "tất", {}});
    queue.push({senkey::ActionType::REPLACEMENT, {}, 2, "cả", {}});
    queue.push({senkey::ActionType::REPLACEMENT, {}, 1, "các", {}});

    // Allow worker to consume
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    queue.stop();
    worker.join();

    assert(received.size() == 3);
    assert(received[0] == "tất");
    assert(received[1] == "cả");
    assert(received[2] == "các");
    std::cout << "[PASS] EventQueue FIFO ordering and thread safety verified.\n";
}

void test_xkb_state() {
    senkey::XkbState xkb;
    senkey::ModifierState mod;
    char c = xkb.process_key(KEY_A, 1, mod);
    assert(c == 'a');
    (void)c;

    mod.lshift = true;
    c = xkb.process_key(KEY_B, 1, mod);
    assert(c == 'B');
    (void)c;

    std::cout << "[PASS] XkbState logical decoding verified.\n";
}

int main() {
    std::cout << "=== Running PipelineQueueTest ===\n";
    test_event_queue();
    test_xkb_state();
    std::cout << "=== PipelineQueueTest PASSED ===\n";
    return 0;
}
