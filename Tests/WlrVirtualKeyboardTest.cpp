// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "Output/Wayland/WlrVirtualKeyboardBackend.hpp"

#ifdef HAVE_XKBCOMMON
#include <xkbcommon/xkbcommon.h>
#include "charset.h"
#endif

using namespace senkey;

void test_graceful_fallback_without_wayland() {
    WlrVirtualKeyboardBackend backend;
    assert(!backend.is_valid());

    // Without a real Wayland compositor on DISPLAY/WAYLAND_DISPLAY in test runner,
    // open_device must return false gracefully and not crash or leak resources.
    bool ok = backend.open_device("senkey-test");
    if (!ok) {
        assert(!backend.is_valid());
        std::cout << "[PASS] Graceful fallback when Wayland compositor is absent.\n";
    } else {
        assert(backend.is_valid());
        std::cout << "[PASS] Wayland backend opened successfully in Wayland environment.\n";
    }

    backend.close_device();
    assert(!backend.is_valid());
}

#ifdef HAVE_XKBCOMMON
void test_static_xkb_keymap_invariance() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    // Build the static Level 0 keymap with identical Level 0 & Level 1 definitions
    std::string xkb = 
        "xkb_keymap {\n"
        "  xkb_keycodes {\n"
        "    include \"evdev+aliases(qwerty)\"\n"
        "    <V_AA> = 200;\n"
        "    <V_OA> = 201;\n"
        "  };\n"
        "  xkb_types {\n"
        "    include \"complete\"\n"
        "    type \"IMMUTABLE\" {\n"
        "      modifiers = Shift + Lock;\n"
        "      map[Shift] = Level1;\n"
        "      map[Lock] = Level1;\n"
        "      map[Shift+Lock] = Level1;\n"
        "      level_name[Level1] = \"Base\";\n"
        "    };\n"
        "  };\n"
        "  xkb_compatibility { include \"complete\" };\n"
        "  xkb_symbols {\n"
        "    include \"pc+us+inet(evdev)\"\n"
        "    key <V_AA> { type = \"IMMUTABLE\", symbols[Group1] = [ U00E1 ] };\n" // á
        "    key <V_OA> { type = \"IMMUTABLE\", symbols[Group1] = [ U00C1 ] };\n" // Á
        "  };\n"
        "};\n";

    struct xkb_keymap* km = xkb_keymap_new_from_string(ctx, xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(km != nullptr);

    struct xkb_state* st = xkb_state_new(km);
    assert(st != nullptr);

    // Verify unshifted state
    char buf[16] = {0};
    xkb_state_key_get_utf8(st, 200, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    xkb_state_key_get_utf8(st, 201, buf, sizeof(buf));
    assert(std::string(buf) == "Á");

    // Simulate Shift modifier depressed -> Must still evaluate to EXACT same character!
    xkb_mod_index_t shift_idx = xkb_keymap_mod_get_index(km, XKB_MOD_NAME_SHIFT);
    xkb_state_update_mask(st, (1 << shift_idx), 0, 0, 0, 0, 0);

    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 200, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 201, buf, sizeof(buf));
    assert(std::string(buf) == "Á");

    // Simulate CapsLock locked -> Must still evaluate to EXACT same character!
    xkb_mod_index_t caps_idx = xkb_keymap_mod_get_index(km, XKB_MOD_NAME_CAPS);
    xkb_state_update_mask(st, 0, 0, (1 << caps_idx), 0, 0, 0);

    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 200, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 201, buf, sizeof(buf));
    assert(std::string(buf) == "Á");

    xkb_state_unref(st);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] Level 0 static keymap invariance under Shift and CapsLock verified.\n";
}

void test_full_generated_xkb_keymap_invariance() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    std::vector<uint32_t> codepoints;
    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    std::string xkb = WlrVirtualKeyboardBackend::build_static_xkb_keymap(codepoints, cp_to_evdev_key);
    assert(!xkb.empty());
    assert(codepoints.size() >= 200); // 95 ASCII + 134+ Vietnamese non-ASCII characters

    struct xkb_keymap* km = xkb_keymap_new_from_string(ctx, xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(km != nullptr);

    struct xkb_state* st = xkb_state_new(km);
    assert(st != nullptr);

    xkb_mod_index_t shift_idx = xkb_keymap_mod_get_index(km, XKB_MOD_NAME_SHIFT);
    xkb_mod_index_t caps_idx = xkb_keymap_mod_get_index(km, XKB_MOD_NAME_CAPS);

    for (uint32_t cp : codepoints) {
        auto it = cp_to_evdev_key.find(cp);
        assert(it != cp_to_evdev_key.end());
        uint32_t xkb_kc = it->second + 8;

        char expected[8] = {0};
        if (cp < 0x80) {
            expected[0] = static_cast<char>(cp);
        } else if (cp < 0x800) {
            expected[0] = static_cast<char>(0xC0 | (cp >> 6));
            expected[1] = static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            expected[0] = static_cast<char>(0xE0 | (cp >> 12));
            expected[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            expected[2] = static_cast<char>(0x80 | (cp & 0x3F));
        }

        // 1. Unshifted state
        xkb_state_update_mask(st, 0, 0, 0, 0, 0, 0);
        char buf[16] = {0};
        xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
        if (std::string(buf) != std::string(expected)) {
            std::cerr << "Mismatch unshifted: got '" << buf << "', expected '" << expected << "'\n";
            std::abort();
        }

        // 2. Shift depressed
        xkb_state_update_mask(st, (1 << shift_idx), 0, 0, 0, 0, 0);
        buf[0] = '\0';
        xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
        if (std::string(buf) != std::string(expected)) {
            std::cerr << "Mismatch with Shift: got '" << buf << "', expected '" << expected << "'\n";
            std::abort();
        }

        // 3. CapsLock locked
        xkb_state_update_mask(st, 0, 0, (1 << caps_idx), 0, 0, 0);
        buf[0] = '\0';
        xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
        if (std::string(buf) != std::string(expected)) {
            std::cerr << "Mismatch with CapsLock: got '" << buf << "', expected '" << expected << "'\n";
            std::abort();
        }

        // 4. Shift depressed + CapsLock locked
        xkb_state_update_mask(st, (1 << shift_idx), 0, (1 << caps_idx), 0, 0, 0);
        buf[0] = '\0';
        xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
        if (std::string(buf) != std::string(expected)) {
            std::cerr << "Mismatch with Shift+CapsLock: got '" << buf << "', expected '" << expected << "'\n";
            std::abort();
        }
    }

    xkb_state_unref(st);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] Full generated XKB keymap (" << codepoints.size()
              << " symbols) verified 100% invariant under unshifted, Shift, CapsLock, and Shift+CapsLock!\n";
}
#endif

int main() {
    std::cout << "Running WlrVirtualKeyboardTest...\n";
    test_graceful_fallback_without_wayland();
#ifdef HAVE_XKBCOMMON
    test_static_xkb_keymap_invariance();
    test_full_generated_xkb_keymap_invariance();
#endif
    std::cout << "All WlrVirtualKeyboardTest cases passed!\n";
    return 0;
}
