// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Output/Wayland/WlrVirtualKeyboardBackend.hpp"
#include "Logger.hpp"
#include "Keymap.hpp"

#ifdef HAVE_WAYLAND
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <set>
#include <vector>
#include <cstdlib>
#include "charset.h" // UniKeyCore UnicodeTable, TOTAL_VNCHARS

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

namespace senkey {

static void seat_keyboard_keymap(void* data, struct wl_keyboard* keyboard,
                                 uint32_t format, int32_t fd, uint32_t size) {
    (void)keyboard;
    auto* self = static_cast<WlrVirtualKeyboardBackend*>(data);
    if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && fd >= 0 && size > 0) {
        char* map_shm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        if (map_shm != MAP_FAILED) {
            std::string km_str(map_shm, size);
            munmap(map_shm, size);
            self->handle_compositor_keymap(km_str);
        }
    }
    if (fd >= 0) close(fd);
}

static void seat_keyboard_enter(void* data, struct wl_keyboard* keyboard,
                               uint32_t serial, struct wl_surface* surface, struct wl_array* keys) {
    (void)data; (void)keyboard; (void)serial; (void)surface; (void)keys;
}

static void seat_keyboard_leave(void* data, struct wl_keyboard* keyboard,
                               uint32_t serial, struct wl_surface* surface) {
    (void)data; (void)keyboard; (void)serial; (void)surface;
}

static void seat_keyboard_key(void* data, struct wl_keyboard* keyboard,
                             uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    (void)data; (void)keyboard; (void)serial; (void)time; (void)key; (void)state;
}

static void seat_keyboard_modifiers(void* data, struct wl_keyboard* keyboard,
                                    uint32_t serial, uint32_t depressed,
                                    uint32_t latched, uint32_t locked, uint32_t group) {
    (void)keyboard; (void)serial; (void)depressed; (void)latched; (void)locked;
    auto* self = static_cast<WlrVirtualKeyboardBackend*>(data);
    self->handle_compositor_modifiers(group);
}

static void seat_keyboard_repeat_info(void* data, struct wl_keyboard* keyboard,
                                     int32_t rate, int32_t delay) {
    (void)data; (void)keyboard; (void)rate; (void)delay;
}

static const struct wl_keyboard_listener seat_keyboard_listener = {
    seat_keyboard_keymap,
    seat_keyboard_enter,
    seat_keyboard_leave,
    seat_keyboard_key,
    seat_keyboard_modifiers,
    seat_keyboard_repeat_info,
};

static void seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t caps) {
    auto* self = static_cast<WlrVirtualKeyboardBackend*>(data);
    self->handle_seat_capabilities(seat, caps);
}

static void seat_handle_name(void* data, struct wl_seat* seat, const char* name) {
    (void)data; (void)seat; (void)name;
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
    seat_handle_name,
};

void WlrVirtualKeyboardBackend::handle_seat_capabilities(struct wl_seat* s, uint32_t caps) {
    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !seat_keyboard) {
        seat_keyboard = wl_seat_get_keyboard(s);
        if (seat_keyboard) {
            wl_keyboard_add_listener(seat_keyboard, &seat_keyboard_listener, this);
        }
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && seat_keyboard) {
        wl_keyboard_destroy(seat_keyboard);
        seat_keyboard = nullptr;
    }
}

void WlrVirtualKeyboardBackend::handle_compositor_keymap(const std::string& keymap_str) {
    compositor_keymap_str = keymap_str;
    Logger::info("WlrVirtualKeyboardBackend: Compositor keymap synchronized (" +
                 std::to_string(keymap_str.size()) + " bytes)");
    if (on_keymap_change_cb) {
        on_keymap_change_cb(keymap_str);
    }
}

void WlrVirtualKeyboardBackend::handle_compositor_modifiers(uint32_t group) {
    if (current_group != group) {
        current_group = group;
        Logger::debug("WlrVirtualKeyboardBackend: Active layout group changed to " + std::to_string(group));
        if (on_group_change_cb) {
            on_group_change_cb(group);
        }
    }
}

void WlrVirtualKeyboardBackend::handle_registry_global(struct wl_registry* reg,
                                                       uint32_t name, const char* interface,
                                                       uint32_t version) {
    if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        if (!seat) {
            seat = static_cast<struct wl_seat*>(
                wl_registry_bind(reg, name, &wl_seat_interface, version <= 7 ? version : 7));
            wl_seat_add_listener(seat, &seat_listener, this);
        }
    } else if (std::strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
        if (!manager) {
            manager = static_cast<struct zwp_virtual_keyboard_manager_v1*>(
                wl_registry_bind(reg, name, &zwp_virtual_keyboard_manager_v1_interface, 1));
        }
    }
}

static void registry_handle_global(void* data, struct wl_registry* reg,
                                   uint32_t name, const char* interface,
                                   uint32_t version) {
    static_cast<WlrVirtualKeyboardBackend*>(data)->handle_registry_global(reg, name, interface, version);
}

static void registry_handle_global_remove(void* data, struct wl_registry* reg, uint32_t name) {
    (void)data;
    (void)reg;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove,
};

bool WlrVirtualKeyboardBackend::init_wayland() {
    const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
    display = wl_display_connect(wayland_display);
    if (!display) {
        return false;
    }

    registry = wl_display_get_registry(display);
    if (!registry) {
        cleanup_wayland();
        return false;
    }

    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);

    if (!manager || !seat) {
        Logger::debug("Wayland compositor does not expose zwp_virtual_keyboard_manager_v1 or wl_seat");
        cleanup_wayland();
        return false;
    }

    // Roundtrip once more to ensure seat capabilities and seat_keyboard listener are processed
    wl_display_roundtrip(display);

    keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(manager, seat);
    if (!keyboard) {
        Logger::debug("Failed to create zwp_virtual_keyboard_v1 instance");
        cleanup_wayland();
        return false;
    }

    if (!setup_static_vietnamese_keymap()) {
        Logger::error("Failed to setup static Level 0 Vietnamese keymap");
        cleanup_wayland();
        return false;
    }

    return true;
}

void WlrVirtualKeyboardBackend::cleanup_wayland() {
    if (seat_keyboard) {
        wl_keyboard_destroy(seat_keyboard);
        seat_keyboard = nullptr;
    }
    if (keyboard) {
        zwp_virtual_keyboard_v1_destroy(keyboard);
        keyboard = nullptr;
    }
    if (manager) {
        zwp_virtual_keyboard_manager_v1_destroy(manager);
        manager = nullptr;
    }
    if (seat) {
        wl_seat_destroy(seat);
        seat = nullptr;
    }
    if (registry) {
        wl_registry_destroy(registry);
        registry = nullptr;
    }
    if (display) {
        wl_display_disconnect(display);
        display = nullptr;
    }
    valid = false;
}

std::string WlrVirtualKeyboardBackend::build_static_xkb_keymap(
    std::vector<uint32_t>& out_codepoints,
    std::unordered_map<uint32_t, uint32_t>& out_cp_to_evdev_key,
    const std::string& base_layout,
    const std::string& base_variant) {
    std::set<uint32_t> codepoints;
    
    // 1. All printable ASCII characters (32 .. 126)
    for (uint32_t c = 32; c <= 126; ++c) {
        codepoints.insert(c);
    }

    // 2. All Vietnamese characters from UniKeyCore UnicodeTable
    for (int i = 0; i < TOTAL_VNCHARS; ++i) {
        uint32_t c = UnicodeTable[i];
        if (c > 127) {
            codepoints.insert(c);
        }
    }

    out_codepoints.assign(codepoints.begin(), codepoints.end());
    out_cp_to_evdev_key.clear();

    // Allocate virtual keys starting at XKB keycode 200 (evdev 192)
    // Private Wayland virtual keyboard device namespace (supports 32-bit keycodes)
    uint32_t start_kc = 200;

    XkbConfig cfg = detect_system_layout_info();
    std::string layout = base_layout.empty() ? cfg.layout : base_layout;
    std::string variant = base_variant.empty() ? cfg.variant : base_variant;
    if (layout.empty()) {
        layout = "us";
    }

    std::string symbols_include = build_symbols_include(layout, variant);

    std::string xkb;
    xkb.reserve(65536);
    xkb += "xkb_keymap {\n";
    xkb += "  xkb_keycodes {\n";
    xkb += "    include \"evdev+aliases(qwerty)\"\n";

    for (size_t i = 0; i < out_codepoints.size(); ++i) {
        uint32_t kc = start_kc + i;
        uint32_t evdev_key = kc - 8;
        out_cp_to_evdev_key[out_codepoints[i]] = evdev_key;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "    <V%03zu> = %u;\n", i, kc);
        xkb += buf;
    }

    xkb += "  };\n";
    xkb += "  xkb_types {\n";
    xkb += "    include \"complete\"\n";
    xkb += "    type \"IMMUTABLE\" {\n";
    xkb += "      modifiers = Shift + Lock;\n";
    xkb += "      map[Shift] = Level1;\n";
    xkb += "      map[Lock] = Level1;\n";
    xkb += "      map[Shift+Lock] = Level1;\n";
    xkb += "      level_name[Level1] = \"Base\";\n";
    xkb += "    };\n";
    xkb += "  };\n";
    xkb += "  xkb_compatibility {\n";
    xkb += "    include \"complete\"\n";
    xkb += "  };\n";
    xkb += "  xkb_symbols {\n";
    xkb += "    include \"" + symbols_include + "\"\n";

    // All Level 0 symbols use type "IMMUTABLE" consuming Shift and Lock:
    // Notice: [ U%04X ] without symbols[Group1] binds to ALL groups (invariant across layout switches)!
    for (size_t i = 0; i < out_codepoints.size(); ++i) {
        uint32_t cp = out_codepoints[i];
        char buf[128];
        std::snprintf(buf, sizeof(buf), "    key <V%03zu> { type = \"IMMUTABLE\", [ U%04X ] };\n", i, cp);
        xkb += buf;
    }

    xkb += "  };\n";
    xkb += "};\n";
    return xkb;
}

bool WlrVirtualKeyboardBackend::setup_static_vietnamese_keymap() {
    std::vector<uint32_t> cp_list;
    std::string xkb = build_static_xkb_keymap(cp_list, cp_to_evdev_key);

    // Allocate anonymous memory file descriptor
    int fd = memfd_create("senkey-xkb", MFD_CLOEXEC);
    if (fd < 0) {
        char tmp_path[] = "/tmp/senkey-xkb-XXXXXX";
        fd = mkstemp(tmp_path);
        if (fd >= 0) {
            unlink(tmp_path);
        }
    }

    if (fd < 0) {
        Logger::error("Failed to allocate memfd/tmpfile for virtual keyboard keymap");
        return false;
    }

    size_t keymap_size = xkb.size() + 1;
    ssize_t written = write(fd, xkb.c_str(), keymap_size);
    if (written < static_cast<ssize_t>(keymap_size)) {
        Logger::error("Failed to write full keymap to memfd");
        close(fd);
        return false;
    }

    zwp_virtual_keyboard_v1_keymap(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, static_cast<uint32_t>(keymap_size));
    wl_display_roundtrip(display);
    close(fd);

    Logger::info("WlrVirtualKeyboardBackend: Uploaded static Level 0 keymap (" + 
                 std::to_string(cp_list.size()) + " symbols) successfully.");
    return true;
}

WlrVirtualKeyboardBackend::WlrVirtualKeyboardBackend() = default;

WlrVirtualKeyboardBackend::~WlrVirtualKeyboardBackend() {
    close_device();
}

bool WlrVirtualKeyboardBackend::open_device(const char* device_name) {
    (void)device_name;
    valid = init_wayland();
    return valid;
}

void WlrVirtualKeyboardBackend::close_device() {
    cleanup_wayland();
}

bool WlrVirtualKeyboardBackend::is_valid() const {
    return valid && keyboard != nullptr;
}

void WlrVirtualKeyboardBackend::emit_passthrough(const struct input_event& ev) {
    if (!valid || !keyboard) return;
    if (ev.type == EV_KEY) {
        uint32_t state = (ev.value > 0) ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;
        if (current_group > 0) {
            zwp_virtual_keyboard_v1_modifiers(keyboard, 0, 0, 0, current_group);
        }
        zwp_virtual_keyboard_v1_key(keyboard, 0, ev.code, state);
        wl_display_flush(display);
    }
}

void WlrVirtualKeyboardBackend::emit_replacement(int backs, const std::string& replacement, const ModifierState& mod) {
    (void)mod;
    if (!valid || !keyboard) return;

    if (current_group > 0) {
        zwp_virtual_keyboard_v1_modifiers(keyboard, 0, 0, 0, current_group);
    }

    // 1. Batched backspaces
    if (backs > 0) {
        for (int i = 0; i < backs; ++i) {
            zwp_virtual_keyboard_v1_key(keyboard, 0, KEY_BACKSPACE, WL_KEYBOARD_KEY_STATE_PRESSED);
            zwp_virtual_keyboard_v1_key(keyboard, 0, KEY_BACKSPACE, WL_KEYBOARD_KEY_STATE_RELEASED);
        }
    }

    if (replacement.empty()) {
        wl_display_flush(display);
        return;
    }

    // 2. Pure Level 0 glyph emission - 0ms delay, zero synthetic Shift!
    const char* ptr = replacement.data();
    const char* end = ptr + replacement.size();
    while (ptr < end) {
        uint32_t cp = utf8_next_codepoint(ptr, end);
        if (cp == 0) break;

        auto it = cp_to_evdev_key.find(cp);
        if (it != cp_to_evdev_key.end()) {
            uint32_t evdev_key = it->second;
            zwp_virtual_keyboard_v1_key(keyboard, 0, evdev_key, WL_KEYBOARD_KEY_STATE_PRESSED);
            zwp_virtual_keyboard_v1_key(keyboard, 0, evdev_key, WL_KEYBOARD_KEY_STATE_RELEASED);
        } else {
            // Fallback for standard ASCII if somehow unmapped
            if (cp < 128) {
                bool shift_needed = false;
                uint16_t sc = ascii_to_scancode(static_cast<char>(cp), shift_needed);
                if (sc > 0) {
                    zwp_virtual_keyboard_v1_key(keyboard, 0, sc, WL_KEYBOARD_KEY_STATE_PRESSED);
                    zwp_virtual_keyboard_v1_key(keyboard, 0, sc, WL_KEYBOARD_KEY_STATE_RELEASED);
                }
            }
        }
    }

    wl_display_flush(display);
}

void WlrVirtualKeyboardBackend::emit_transaction(const TextTransaction& tx) {
    emit_replacement(tx.backs, tx.utf8, tx.mod);
}

} // namespace senkey

#else

namespace senkey {

std::string WlrVirtualKeyboardBackend::build_static_xkb_keymap(
    std::vector<uint32_t>& out_codepoints,
    std::unordered_map<uint32_t, uint32_t>& out_cp_to_evdev_key,
    const std::string& base_layout,
    const std::string& base_variant) {
    (void)out_codepoints;
    (void)out_cp_to_evdev_key;
    (void)base_layout;
    (void)base_variant;
    return "";
}

WlrVirtualKeyboardBackend::WlrVirtualKeyboardBackend() = default;
WlrVirtualKeyboardBackend::~WlrVirtualKeyboardBackend() = default;
bool WlrVirtualKeyboardBackend::open_device(const char* device_name) { (void)device_name; return false; }
void WlrVirtualKeyboardBackend::close_device() {}
bool WlrVirtualKeyboardBackend::is_valid() const { return false; }
void WlrVirtualKeyboardBackend::emit_passthrough(const struct input_event& ev) { (void)ev; }
void WlrVirtualKeyboardBackend::emit_replacement(int backs, const std::string& replacement, const ModifierState& mod) { (void)backs; (void)replacement; (void)mod; }
void WlrVirtualKeyboardBackend::emit_transaction(const TextTransaction& tx) { (void)tx; }

} // namespace senkey

#endif
