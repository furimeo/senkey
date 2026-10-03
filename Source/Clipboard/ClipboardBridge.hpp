// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdint>

struct _XDisplay;
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef XID Atom;

namespace senkey {

class ClipboardBridge {
private:
    Display* dpy = nullptr;
    Window win = 0;
    Atom clipboard_atom = 0;
    Atom utf8_atom = 0;
    Atom targets_atom = 0;
    Atom prop_atom = 0;

    std::string saved_text;
    std::string staged_text;
    std::atomic<bool> serving_staged{false};
    std::atomic<bool> request_served{false};
    std::mutex clip_mutex;

    void init_x11();
    void cleanup_x11();

public:
    ClipboardBridge();
    ~ClipboardBridge();

    ClipboardBridge(const ClipboardBridge&) = delete;
    ClipboardBridge& operator=(const ClipboardBridge&) = delete;

    bool is_valid() const;

    // Snapshot user's current clipboard text
    std::string get_current_text(int timeout_ms = 35);

    // Stage replacement text and claim clipboard ownership
    void stage_text(const std::string& text);

    // Process event loop until target application pastes staged text
    bool process_events_until_pasted(int max_wait_ms = 35);

    // Restore user's original clipboard
    void restore_saved();

    // Set saved text manually
    void set_saved_text(const std::string& text) {
        std::lock_guard<std::mutex> lock(clip_mutex);
        saved_text = text;
    }
};

} // namespace senkey
