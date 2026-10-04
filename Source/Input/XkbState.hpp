// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <fstream>
#include <cstdlib>
#ifdef HAVE_XKBCOMMON
#include <xkbcommon/xkbcommon.h>
#endif
#include "Keymap.hpp"

namespace senkey {

struct ScancodeShift {
    uint16_t code = 0;
    bool shift = false;
};

class XkbState {
private:
#ifdef HAVE_XKBCOMMON
    struct xkb_context* ctx = nullptr;
    struct xkb_keymap* keymap = nullptr;
    struct xkb_state* state = nullptr;
    xkb_mod_index_t shift_idx = XKB_MOD_INVALID;
    xkb_mod_index_t caps_idx = XKB_MOD_INVALID;
#endif
    std::unordered_map<char, ScancodeShift> reverse_map;

    std::string detect_system_layout() {
        return senkey::detect_system_layout();
    }

    void build_reverse_map() {
        reverse_map.clear();
#ifdef HAVE_XKBCOMMON
        if (!keymap) return;
        xkb_keycode_t min_kc = xkb_keymap_min_keycode(keymap);
        xkb_keycode_t max_kc = xkb_keymap_max_keycode(keymap);

        for (xkb_keycode_t kc = min_kc; kc <= max_kc; ++kc) {
            if (kc < 8) continue;
            uint16_t evdev_code = static_cast<uint16_t>(kc - 8);

            // Level 0: Phím thường không Shift
            const xkb_keysym_t* syms = nullptr;
            int nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, 0, 0, &syms);
            if (nsyms == 1 && syms[0] > 0 && syms[0] < 128) {
                char c = static_cast<char>(syms[0]);
                if (reverse_map.find(c) == reverse_map.end()) {
                    reverse_map[c] = { evdev_code, false };
                }
            }

            // Level 1: Phím khi nhấn cùng Shift
            nsyms = xkb_keymap_key_get_syms_by_level(keymap, kc, 0, 1, &syms);
            if (nsyms == 1 && syms[0] > 0 && syms[0] < 128) {
                char c = static_cast<char>(syms[0]);
                if (reverse_map.find(c) == reverse_map.end()) {
                    reverse_map[c] = { evdev_code, true };
                }
            }
        }
#endif
    }

public:
    XkbState() {
#ifdef HAVE_XKBCOMMON
        ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (ctx) {
            struct xkb_rule_names names = {};
            std::string layout = detect_system_layout();
            if (!layout.empty()) {
                names.layout = layout.c_str();
            }
            keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
            if (keymap) {
                state = xkb_state_new(keymap);
                shift_idx = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
                caps_idx = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
                build_reverse_map();
            }
        }
#endif
    }

    ~XkbState() {
#ifdef HAVE_XKBCOMMON
        if (state) xkb_state_unref(state);
        if (keymap) xkb_keymap_unref(keymap);
        if (ctx) xkb_context_unref(ctx);
#endif
    }

    bool get_scancode_for_char(char c, uint16_t& out_code, bool& out_shift) const {
        auto it = reverse_map.find(c);
        if (it != reverse_map.end()) {
            out_code = it->second.code;
            out_shift = it->second.shift;
            return true;
        }
        return false;
    }

    char process_key(uint16_t evdev_code, int val, const ModifierState& mod) {
#ifdef HAVE_XKBCOMMON
        if (state) {
            xkb_keycode_t xkb_kc = static_cast<xkb_keycode_t>(evdev_code + 8);
            if (val == 1) { // Key down
                xkb_mod_mask_t depressed = 0;
                xkb_mod_mask_t locked = 0;
                if (mod.any_shift() && shift_idx != XKB_MOD_INVALID) {
                    depressed |= (1u << shift_idx);
                }
                if (mod.capslock && caps_idx != XKB_MOD_INVALID) {
                    locked |= (1u << caps_idx);
                }
                xkb_state_update_mask(state, depressed, 0, locked, 0, 0, 0);

                char buf[16] = {0};
                xkb_state_key_get_utf8(state, xkb_kc, buf, sizeof(buf));
                if (buf[0] != '\0' && buf[1] == '\0') {
                    return buf[0];
                }
            }
            return 0;
        }
#endif
        if (val == 1) {
            return scancode_to_ascii(evdev_code, mod.any_shift(), mod.capslock);
        }
        return 0;
    }

    std::string process_key_utf8(uint16_t evdev_code, int val, const ModifierState& mod) {
#ifdef HAVE_XKBCOMMON
        if (state) {
            xkb_keycode_t xkb_kc = static_cast<xkb_keycode_t>(evdev_code + 8);
            if (val == 1) {
                xkb_mod_mask_t depressed = 0;
                xkb_mod_mask_t locked = 0;
                if (mod.any_shift() && shift_idx != XKB_MOD_INVALID) {
                    depressed |= (1u << shift_idx);
                }
                if (mod.capslock && caps_idx != XKB_MOD_INVALID) {
                    locked |= (1u << caps_idx);
                }
                xkb_state_update_mask(state, depressed, 0, locked, 0, 0, 0);

                char buf[32] = {0};
                int len = xkb_state_key_get_utf8(state, xkb_kc, buf, sizeof(buf));
                if (len > 0) {
                    return std::string(buf, len);
                }
            }
            return "";
        }
#endif
        char c = process_key(evdev_code, val, mod);
        if (c != 0) {
            return std::string(1, c);
        }
        return "";
    }
};

} // namespace senkey
