// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <linux/input.h>
#include <string>
#include <cstdint>
#include "Keymap.hpp"

namespace senkey {

struct TextTransaction {
    int backs = 0;
    std::string utf8;
    ModifierState mod{};
};

class OutputBackend {
public:
    virtual ~OutputBackend() = default;
    virtual bool open_device(const char* device_name = "senkey-keyboard") = 0;
    virtual void close_device() = 0;
    virtual bool is_valid() const = 0;
    virtual void emit_passthrough(const struct input_event& ev) = 0;
    virtual void emit_replacement(int backs, const std::string& replacement, const ModifierState& mod) = 0;
    virtual void emit_transaction(const TextTransaction& tx) {
        emit_replacement(tx.backs, tx.utf8, tx.mod);
    }

    // Event loop integration (e.g. Wayland display socket multiplexing)
    virtual int get_poll_fd() const { return -1; }
    virtual bool prepare_read() { return false; }
    virtual void read_events() {}
    virtual void cancel_read() {}
    virtual int dispatch_pending() { return 0; }
};

} // namespace senkey
