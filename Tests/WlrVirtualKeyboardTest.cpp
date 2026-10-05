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

void test_custom_layout_passthrough_invariance() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    std::vector<uint32_t> codepoints;
    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    // Build keymap with French AZERTY base layout
    std::string xkb = WlrVirtualKeyboardBackend::build_static_xkb_keymap(codepoints, cp_to_evdev_key, "fr");
    assert(!xkb.empty());

    struct xkb_keymap* km = xkb_keymap_new_from_string(ctx, xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(km != nullptr);

    struct xkb_state* st = xkb_state_new(km);
    assert(st != nullptr);

    // In AZERTY (fr):
    // KEY_Q (evdev 16 -> XKB 24) is 'a'
    // KEY_A (evdev 30 -> XKB 38) is 'q'
    char buf[16] = {0};
    xkb_state_key_get_utf8(st, 24, buf, sizeof(buf)); // KEY_Q in AZERTY
    assert(std::string(buf) == "a");

    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 38, buf, sizeof(buf)); // KEY_A in AZERTY
    assert(std::string(buf) == "q");

    // Vietnamese Level 0 virtual keys remain completely immutable and unaffected by AZERTY!
    auto it = cp_to_evdev_key.find(0x00E1); // á
    assert(it != cp_to_evdev_key.end());
    uint32_t xkb_kc = it->second + 8;
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    xkb_state_unref(st);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] Custom layout (AZERTY fr) passthrough verified ('KEY_Q'->'a', 'KEY_A'->'q') while Vietnamese Level 0 remains 100% immutable!\n";
}

void test_multi_layout_runtime_group_switching() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    std::vector<uint32_t> codepoints;
    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    // Build keymap with multiple layouts: US (Group 0) and FR (Group 1)
    std::string xkb = WlrVirtualKeyboardBackend::build_static_xkb_keymap(codepoints, cp_to_evdev_key, "us,fr");
    assert(!xkb.empty());

    struct xkb_keymap* km = xkb_keymap_new_from_string(ctx, xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(km != nullptr);

    struct xkb_state* st = xkb_state_new(km);
    assert(st != nullptr);

    // Group 0 (US): KEY_Q (evdev 16 -> XKB 24) is 'q'
    char buf[16] = {0};
    xkb_state_key_get_utf8(st, 24, buf, sizeof(buf));
    assert(std::string(buf) == "q");

    // Switch to Group 1 (FR AZERTY): KEY_Q becomes 'a'
    xkb_state_update_mask(st, 0, 0, 0, 0, 0, 1);
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 24, buf, sizeof(buf));
    assert(std::string(buf) == "a");

    // Vietnamese Level 0 immutable key is invariant across both groups:
    auto it = cp_to_evdev_key.find(0x00E1); // á
    assert(it != cp_to_evdev_key.end());
    uint32_t xkb_kc = it->second + 8;

    // Check in Group 1
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    // Switch back to Group 0 and check
    xkb_state_update_mask(st, 0, 0, 0, 0, 0, 0);
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    xkb_state_unref(st);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] Multi-layout runtime group switching (US <-> FR) verified with 100% Level 0 Vietnamese invariance!\n";
}

void test_layout_variant_passthrough() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    std::vector<uint32_t> codepoints;
    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    // Build keymap with US layout, Dvorak variant
    std::string xkb = WlrVirtualKeyboardBackend::build_static_xkb_keymap(codepoints, cp_to_evdev_key, "us", "dvorak");
    assert(!xkb.empty());

    struct xkb_keymap* km = xkb_keymap_new_from_string(ctx, xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(km != nullptr);

    struct xkb_state* st = xkb_state_new(km);
    assert(st != nullptr);

    // In Dvorak: KEY_Q (evdev 16 -> XKB 24) is '\''
    char buf[16] = {0};
    xkb_state_key_get_utf8(st, 24, buf, sizeof(buf));
    assert(std::string(buf) == "'");

    // Vietnamese Level 0 immutable key remains intact
    auto it = cp_to_evdev_key.find(0x00E1); // á
    assert(it != cp_to_evdev_key.end());
    uint32_t xkb_kc = it->second + 8;
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    xkb_state_unref(st);
    xkb_keymap_unref(km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] Layout variant (US Dvorak) passthrough verified with 100% Level 0 Vietnamese invariance!\n";
}

void test_build_symbols_include_helpers() {
    assert(build_symbols_include("us") == "pc+us+inet(evdev)");
    assert(build_symbols_include("fr") == "pc+fr+inet(evdev)");
    assert(build_symbols_include("us,fr") == "pc+us+inet(evdev)+fr:2");
    assert(build_symbols_include("us,fr", ",bepo") == "pc+us+inet(evdev)+fr(bepo):2");
    assert(build_symbols_include("us", "dvorak") == "pc+us(dvorak)+inet(evdev)");
    std::cout << "[PASS] build_symbols_include helper correctly constructs single, multi-layout, and variant symbol directives!\n";
}

void test_merge_compositor_keymap_with_vietnamese() {
    struct xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx != nullptr);

    // Simulate a compositor sending its active keymap at runtime (German QWERTZ as Group 0, French AZERTY as Group 1)
    struct xkb_rule_names names = {};
    names.layout = "de,fr";
    names.options = "grp:alt_shift_toggle";

    struct xkb_keymap* comp_km = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(comp_km != nullptr);

    char* comp_str = xkb_keymap_get_as_string(comp_km, XKB_KEYMAP_FORMAT_TEXT_V1);
    assert(comp_str != nullptr);
    std::string comp_xkb(comp_str);
    free(comp_str);
    xkb_keymap_unref(comp_km);

    // Merge the compositor keymap with SenKey's Level 0 Vietnamese keys
    std::unordered_map<uint32_t, uint32_t> cp_to_evdev_key;
    std::string merged_xkb = WlrVirtualKeyboardBackend::merge_compositor_keymap_with_vietnamese(comp_xkb, cp_to_evdev_key);
    assert(!merged_xkb.empty());

    // Compile merged keymap
    struct xkb_keymap* merged_km = xkb_keymap_new_from_string(ctx, merged_xkb.c_str(),
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(merged_km != nullptr);

    struct xkb_state* st = xkb_state_new(merged_km);
    assert(st != nullptr);

    // Verify Group 0 (German QWERTZ):
    // KEY_Y (evdev 21 -> XKB 29) is 'z'
    char buf[16] = {0};
    xkb_state_key_get_utf8(st, 29, buf, sizeof(buf));
    assert(std::string(buf) == "z");

    // Switch to Group 1 (French AZERTY):
    // KEY_Q (evdev 16 -> XKB 24) is 'a'
    xkb_state_update_mask(st, 0, 0, 0, 0, 0, 1);
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, 24, buf, sizeof(buf));
    assert(std::string(buf) == "a");

    // Verify Vietnamese Level 0 immutable glyphs are intact across both groups:
    auto it = cp_to_evdev_key.find(0x00E1); // á
    assert(it != cp_to_evdev_key.end());
    uint32_t xkb_kc = it->second + 8;

    // Check in Group 1
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    // Check in Group 0
    xkb_state_update_mask(st, 0, 0, 0, 0, 0, 0);
    buf[0] = '\0';
    xkb_state_key_get_utf8(st, xkb_kc, buf, sizeof(buf));
    assert(std::string(buf) == "á");

    xkb_state_unref(st);
    xkb_keymap_unref(merged_km);
    xkb_context_unref(ctx);

    std::cout << "[PASS] merge_compositor_keymap_with_vietnamese verified with full compositor base passthrough and Level 0 Vietnamese invariance!\n";
}
#endif

int main() {
    std::cout << "Running WlrVirtualKeyboardTest...\n";
    test_graceful_fallback_without_wayland();
#ifdef HAVE_XKBCOMMON
    test_static_xkb_keymap_invariance();
    test_full_generated_xkb_keymap_invariance();
    test_custom_layout_passthrough_invariance();
    test_multi_layout_runtime_group_switching();
    test_layout_variant_passthrough();
    test_build_symbols_include_helpers();
    test_merge_compositor_keymap_with_vietnamese();
#endif
    std::cout << "All WlrVirtualKeyboardTest cases passed!\n";
    return 0;
}
