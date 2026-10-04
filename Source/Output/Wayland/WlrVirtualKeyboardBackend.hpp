// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <linux/input.h>
#include "Output/OutputBackend.hpp"

#ifdef HAVE_WAYLAND
#include <wayland-client.h>
#include "Output/Wayland/virtual-keyboard-unstable-v1-client-protocol.h"
#endif

namespace senkey {

class WlrVirtualKeyboardBackend : public OutputBackend {
private:
#ifdef HAVE_WAYLAND
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    struct wl_seat* seat = nullptr;
    struct zwp_virtual_keyboard_manager_v1* manager = nullptr;
    struct zwp_virtual_keyboard_v1* keyboard = nullptr;

    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    bool valid = false;

    bool init_wayland();
    void cleanup_wayland();
    bool setup_static_vietnamese_keymap();
#endif

public:
    WlrVirtualKeyboardBackend();
    ~WlrVirtualKeyboardBackend() override;

#ifdef HAVE_WAYLAND
    void handle_registry_global(struct wl_registry* reg, uint32_t name, const char* interface, uint32_t version);
#endif

    bool open_device(const char* device_name = "senkey-keyboard") override;
    void close_device() override;
    bool is_valid() const override;

    void emit_passthrough(const struct input_event& ev) override;
    void emit_replacement(int backs, const std::string& replacement, const ModifierState& mod) override;
    void emit_transaction(const TextTransaction& tx) override;
};

} // namespace senkey
