// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once

#include <cstdint>
#ifdef HAVE_XKBCOMMON
#include <xkbcommon/xkbcommon.h>
#endif
#include "Keymap.hpp"

namespace senkey {

class XkbState {
private:
#ifdef HAVE_XKBCOMMON
    struct xkb_context* ctx = nullptr;
    struct xkb_keymap* keymap = nullptr;
    struct xkb_state* state = nullptr;
    xkb_mod_index_t shift_idx = XKB_MOD_INVALID;
    xkb_mod_index_t caps_idx = XKB_MOD_INVALID;
#endif

public:
    XkbState() {
#ifdef HAVE_XKBCOMMON
        ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (ctx) {
            struct xkb_rule_names names = {};
            keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
            if (keymap) {
                state = xkb_state_new(keymap);
                shift_idx = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
                caps_idx = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
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
};

} // namespace senkey
