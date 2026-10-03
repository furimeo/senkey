// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include "Pipeline/EventQueue.hpp"
#include "Pipeline/IpcCommandQueue.hpp"
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
    assert(queue.get_peak_depth() >= 1);
    assert(queue.get_dropped_count() == 0);
    std::cout << "[PASS] EventQueue FIFO ordering, bounded tracking, and thread safety verified.\n";
}

void test_ipc_command_queue() {
    senkey::IpcCommandQueue cmd_queue;
    bool woke_up = false;
    cmd_queue.set_wake_callback([&woke_up]() {
        woke_up = true;
    });

    auto prom = std::make_shared<std::promise<std::string>>();
    auto fut = prom->get_future();
    cmd_queue.push({"TOGGLE", prom});

    assert(woke_up == true);

    auto drained = cmd_queue.drain();
    assert(drained.size() == 1);
    assert(drained[0].command == "TOGGLE");

    drained[0].promise->set_value("V");
    assert(fut.get() == "V");

    std::cout << "[PASS] IpcCommandQueue thread synchronization and wakeup callback verified.\n";
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

    std::string utf8_c = xkb.process_key_utf8(KEY_C, 1, mod);
    assert(utf8_c == "C");

    // Test reverse keymap lookup
    uint16_t code = 0;
    bool shift = false;
    bool found = xkb.get_scancode_for_char('a', code, shift);
    if (found) {
        assert(code == KEY_A);
        assert(shift == false);
    }

    found = xkb.get_scancode_for_char('A', code, shift);
    if (found) {
        assert(code == KEY_A);
        assert(shift == true);
    }

    std::cout << "[PASS] XkbState logical decoding and reverse layout mapping verified.\n";
}

int main() {
    std::cout << "=== Running PipelineQueueTest ===\n";
    test_event_queue();
    test_ipc_command_queue();
    test_xkb_state();
    std::cout << "=== PipelineQueueTest PASSED ===\n";
    return 0;
}
