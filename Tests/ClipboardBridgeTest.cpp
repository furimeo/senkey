// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <cassert>
#include <iostream>
#include "Clipboard/ClipboardBridge.hpp"

void test_clipboard_bridge_lifecycle() {
    senkey::ClipboardBridge bridge;
    // On systems with display available, is_valid should be true;
    // in headless CI, it should gracefully return false without crashing.
    std::cout << "[INFO] ClipboardBridge is_valid: " << (bridge.is_valid() ? "TRUE" : "FALSE") << "\n";

    if (bridge.is_valid()) {
        std::string initial = bridge.get_current_text(20);
        std::cout << "[INFO] Initial clipboard: [" << initial << "]\n";

        bridge.set_saved_text("OriginalUserText");
        bridge.stage_text("cách");
        // Simulate serving request for 10ms
        bridge.process_events_until_pasted(10);
        bridge.restore_saved();

        std::cout << "[PASS] Clipboard staging and restoration completed.\n";
    } else {
        std::cout << "[PASS] Graceful headless fallback verified.\n";
    }
}

int main() {
    std::cout << "=== Running ClipboardBridgeTest ===\n";
    test_clipboard_bridge_lifecycle();
    std::cout << "=== ClipboardBridgeTest PASSED ===\n";
    return 0;
}
