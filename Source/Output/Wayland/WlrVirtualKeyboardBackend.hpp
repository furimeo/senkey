// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
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
    struct wl_keyboard* seat_keyboard = nullptr;
    struct zwp_virtual_keyboard_manager_v1* manager = nullptr;
    struct zwp_virtual_keyboard_v1* keyboard = nullptr;

    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    bool valid = false;
    uint32_t current_group = 0;
    std::string compositor_keymap_str;
    std::function<void(uint32_t)> on_group_change_cb;
    std::function<void(const std::string&)> on_keymap_change_cb;

    bool init_wayland();
    void cleanup_wayland();
    bool setup_static_vietnamese_keymap();
#endif

public:
    WlrVirtualKeyboardBackend();
    ~WlrVirtualKeyboardBackend() override;

    static std::string build_static_xkb_keymap(
        std::vector<uint32_t>& out_codepoints,
        std::unordered_map<uint32_t, uint32_t>& out_cp_to_evdev_key,
        const std::string& base_layout = "",
        const std::string& base_variant = "");

#ifdef HAVE_WAYLAND
    void handle_registry_global(struct wl_registry* reg, uint32_t name, const char* interface, uint32_t version);
    void handle_seat_capabilities(struct wl_seat* seat, uint32_t capabilities);
    void handle_compositor_keymap(const std::string& keymap_str);
    void handle_compositor_modifiers(uint32_t group);
#endif

    void set_on_group_change(std::function<void(uint32_t)> cb) {
#ifdef HAVE_WAYLAND
        on_group_change_cb = std::move(cb);
#else
        (void)cb;
#endif
    }

    void set_on_keymap_change(std::function<void(const std::string&)> cb) {
#ifdef HAVE_WAYLAND
        on_keymap_change_cb = std::move(cb);
#else
        (void)cb;
#endif
    }

    uint32_t get_group() const {
#ifdef HAVE_WAYLAND
        return current_group;
#else
        return 0;
#endif
    }

    bool open_device(const char* device_name = "senkey-keyboard") override;
    void close_device() override;
    bool is_valid() const override;

    void emit_passthrough(const struct input_event& ev) override;
    void emit_replacement(int backs, const std::string& replacement, const ModifierState& mod) override;
    void emit_transaction(const TextTransaction& tx) override;
};

} // namespace senkey
