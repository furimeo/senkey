// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdint>
#include <string>
#include <chrono>
#include <thread>
#include "Keymap.hpp"
#include "Clipboard/ClipboardBridge.hpp"

namespace senkey {

class VirtualKeyboard {
private:
    int uinput_fd = -1;
    ClipboardBridge clipboard_bridge;

    void sleep_us(int us) {
        if (us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(us));
        }
    }

public:
    VirtualKeyboard() = default;

    ~VirtualKeyboard() {
        close_device();
    }

    VirtualKeyboard(const VirtualKeyboard&) = delete;
    VirtualKeyboard& operator=(const VirtualKeyboard&) = delete;

    bool open_device(const char* device_name = "senkey-keyboard") {
        uinput_fd = open("/dev/uinput", O_WRONLY);
        if (uinput_fd < 0) {
            uinput_fd = open("/dev/input/uinput", O_WRONLY);
        }
        if (uinput_fd < 0) {
            return false;
        }

        ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
        ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);

        for (int k = 1; k < 255; ++k) {
            ioctl(uinput_fd, UI_SET_KEYBIT, k);
        }

        struct uinput_setup setup;
        std::memset(&setup, 0, sizeof(setup));
        std::strncpy(setup.name, device_name, UINPUT_MAX_NAME_SIZE - 1);
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x1234;
        setup.id.product = 0x5678;
        setup.id.version = 1;

        if (ioctl(uinput_fd, UI_DEV_SETUP, &setup) < 0) {
            close_device();
            return false;
        }

        if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
            close_device();
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    }

    void close_device() {
        if (uinput_fd >= 0) {
            ioctl(uinput_fd, UI_DEV_DESTROY);
            close(uinput_fd);
            uinput_fd = -1;
        }
    }

    bool is_valid() const {
        return uinput_fd >= 0;
    }

    void emit_event(uint16_t type, uint16_t code, int32_t value) {
        if (uinput_fd < 0) return;
        struct input_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.type = type;
        ev.code = code;
        ev.value = value;
        while (write(uinput_fd, &ev, sizeof(ev)) < 0) {
            if (errno == EINTR) continue;
            break;
        }
    }

    void sync() {
        emit_event(EV_SYN, SYN_REPORT, 0);
    }

    void passthrough(const struct input_event& ev) {
        if (uinput_fd < 0) return;
        while (write(uinput_fd, &ev, sizeof(ev)) < 0) {
            if (errno == EINTR) continue;
            break;
        }
    }

    void tap_key(int code, int delay_us = 600) {
        if (uinput_fd < 0 || code <= 0) return;
        emit_event(EV_KEY, code, 1);
        sync();
        sleep_us(delay_us);
        emit_event(EV_KEY, code, 0);
        sync();
        sleep_us(delay_us);
    }

    void emit_backspaces(int count, int delay_us = 600) {
        for (int i = 0; i < count; ++i) {
            tap_key(KEY_BACKSPACE, delay_us);
        }
    }

    void emit_paste_shift_insert(int delay_us = 1200) {
        emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
        sync();
        sleep_us(delay_us);
        tap_key(KEY_INSERT, delay_us);
        emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
        sync();
        sleep_us(delay_us);
    }

    ClipboardBridge& get_clipboard_bridge() { return clipboard_bridge; }
    const ClipboardBridge& get_clipboard_bridge() const { return clipboard_bridge; }

    void release_all_modifiers(const ModifierState& mod) {
        bool changed = false;
        if (mod.lshift) { emit_event(EV_KEY, KEY_LEFTSHIFT, 0); changed = true; }
        if (mod.rshift) { emit_event(EV_KEY, KEY_RIGHTSHIFT, 0); changed = true; }
        if (mod.lctrl)  { emit_event(EV_KEY, KEY_LEFTCTRL, 0); changed = true; }
        if (mod.rctrl)  { emit_event(EV_KEY, KEY_RIGHTCTRL, 0); changed = true; }
        if (mod.lalt)   { emit_event(EV_KEY, KEY_LEFTALT, 0); changed = true; }
        if (mod.ralt)   { emit_event(EV_KEY, KEY_RIGHTALT, 0); changed = true; }
        if (mod.super)  {
            emit_event(EV_KEY, KEY_LEFTMETA, 0);
            emit_event(EV_KEY, KEY_RIGHTMETA, 0);
            changed = true;
        }
        if (mod.capslock) {
            // Temporarily disable CapsLock in uinput so Chromium/GTK don't reject Ctrl+Shift+U
            tap_key(KEY_CAPSLOCK, 250);
            changed = true;
        }
        if (changed) {
            sync();
            sleep_us(300);
        }
    }

    void restore_all_modifiers(const ModifierState& mod) {
        bool changed = false;
        if (mod.capslock) {
            // Restore CapsLock state in uinput
            tap_key(KEY_CAPSLOCK, 250);
            changed = true;
        }
        if (mod.lshift) { emit_event(EV_KEY, KEY_LEFTSHIFT, 1); changed = true; }
        if (mod.rshift) { emit_event(EV_KEY, KEY_RIGHTSHIFT, 1); changed = true; }
        if (mod.lctrl)  { emit_event(EV_KEY, KEY_LEFTCTRL, 1); changed = true; }
        if (mod.rctrl)  { emit_event(EV_KEY, KEY_RIGHTCTRL, 1); changed = true; }
        if (mod.lalt)   { emit_event(EV_KEY, KEY_LEFTALT, 1); changed = true; }
        if (mod.ralt)   { emit_event(EV_KEY, KEY_RIGHTALT, 1); changed = true; }
        if (mod.super)  { emit_event(EV_KEY, KEY_LEFTMETA, 1); changed = true; }
        if (changed) {
            sync();
            sleep_us(300);
        }
    }

    bool emit_ascii_char(char c, int delay_us = 600, const ModifierState& mod = {}) {
        bool shift_needed = false;
        int code = ascii_to_scancode(c, shift_needed);
        if (code <= 0) return false;

        bool had_lshift = mod.lshift;
        bool had_rshift = mod.rshift;
        bool had_ctrl = mod.any_ctrl();
        bool had_alt = mod.any_alt();

        // Release interfering modifiers
        if (had_ctrl) {
            if (mod.lctrl) emit_event(EV_KEY, KEY_LEFTCTRL, 0);
            if (mod.rctrl) emit_event(EV_KEY, KEY_RIGHTCTRL, 0);
        }
        if (had_alt) {
            if (mod.lalt) emit_event(EV_KEY, KEY_LEFTALT, 0);
            if (mod.ralt) emit_event(EV_KEY, KEY_RIGHTALT, 0);
        }

        bool current_shift = (had_lshift || had_rshift);
        if (current_shift != shift_needed) {
            if (!shift_needed) {
                if (had_lshift) emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                if (had_rshift) emit_event(EV_KEY, KEY_RIGHTSHIFT, 0);
            } else {
                emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
            }
            sync();
            sleep_us(delay_us);
        }

        tap_key(code, delay_us);

        // Restore shift
        if (current_shift != shift_needed) {
            if (!shift_needed) {
                if (had_lshift) emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                if (had_rshift) emit_event(EV_KEY, KEY_RIGHTSHIFT, 1);
            } else {
                emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
            }
            sync();
            sleep_us(delay_us);
        }

        // Restore ctrl and alt
        if (had_ctrl) {
            if (mod.lctrl) emit_event(EV_KEY, KEY_LEFTCTRL, 1);
            if (mod.rctrl) emit_event(EV_KEY, KEY_RIGHTCTRL, 1);
        }
        if (had_alt) {
            if (mod.lalt) emit_event(EV_KEY, KEY_LEFTALT, 1);
            if (mod.ralt) emit_event(EV_KEY, KEY_RIGHTALT, 1);
        }
        if (had_ctrl || had_alt) {
            sync();
            sleep_us(delay_us);
        }

        return true;
    }

    void emit_unicode(uint32_t codepoint, int delay_us = 1200, const ModifierState& mod = {}) {
        if (codepoint < 128) {
            emit_ascii_char(static_cast<char>(codepoint), delay_us, mod);
            return;
        }

        int step_delay = std::max(delay_us, 1200);

        // 1. Release ALL physical modifiers and CapsLock to ensure clean environment
        release_all_modifiers(mod);

        // 2. Trigger Ctrl+Shift+U with solid, detectable pulse (Shift pressed first, released last to prevent Ctrl+U)
        emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
        emit_event(EV_KEY, KEY_LEFTCTRL, 1);
        sync();
        sleep_us(2500);

        tap_key(KEY_U, 2000);

        emit_event(EV_KEY, KEY_LEFTCTRL, 0);
        emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
        sync();
        sleep_us(2000);

        // 3. Emit Hex Codepoint Digits (lowercase, solid pulse)
        char hex_buf[16];
        std::snprintf(hex_buf, sizeof(hex_buf), "%x", codepoint);

        for (int i = 0; hex_buf[i] != '\0'; ++i) {
            int sc = hex_char_to_scancode(hex_buf[i]);
            if (sc > 0) {
                tap_key(sc, step_delay);
            }
        }

        // 4. Commit via Space (never triggers form or chat send, user preferred)
        tap_key(KEY_SPACE, 1500);

        // 5. Post-commit settling barrier: give target application event loops (Chrome, GTK, Qt)
        // sufficient time (~20ms) to commit the unicode glyph and destroy preedit widget
        sleep_us(20000);

        // 6. Restore physical modifier states
        restore_all_modifiers(mod);
    }

    void emit_utf8_string(const std::string& str, int delay_us = 1200, const ModifierState& mod = {}) {
        const char* ptr = str.data();
        const char* end = ptr + str.size();
        while (ptr < end) {
            uint32_t cp = utf8_next_codepoint(ptr, end);
            if (cp == 0) break;
            emit_unicode(cp, delay_us, mod);
            sleep_us(delay_us);
        }
    }

    void emit_replacement(int backs, const std::string& str, int delay_us = 1200, const ModifierState& mod = {}) {
        if (backs <= 0 && str.empty()) return;

        int step_delay = std::max(delay_us, 1200);

        // Perform entire backspace + replacement under released modifier protection
        release_all_modifiers(mod);

        // ATOMIC CLIPBOARD SWAP VIA SHIFT+INSERT (Ultra-fast, zero-flicker, 100% reliable)
        if (clipboard_bridge.is_valid() && !str.empty()) {
            std::string orig_clip = clipboard_bridge.get_current_text(25);
            clipboard_bridge.set_saved_text(orig_clip);
            clipboard_bridge.stage_text(str);

            if (backs > 0) {
                for (int i = 0; i < backs; ++i) {
                    tap_key(KEY_BACKSPACE, 1000);
                }
                sleep_us(3000);
            }

            emit_paste_shift_insert(step_delay);
            clipboard_bridge.process_events_until_pasted(35);
            clipboard_bridge.restore_saved();

            restore_all_modifiers(mod);
            return;
        }

        // FALLBACK: ISO 14755 Unicode Hex Sequence (when no display connection / headless)
        if (backs > 0) {
            for (int i = 0; i < backs; ++i) {
                tap_key(KEY_BACKSPACE, 1000);
            }
            // CRITICAL: Settling barrier after backspaces!
            // Must allow the X11/Wayland compositor and application event loop a full frame (~18ms)
            // to completely finish and commit all backspace deletions BEFORE pressing KEY_LEFTCTRL!
            // If Ctrl is pressed too quickly (<5ms), compositors batch Backspace and Ctrl together,
            // triggering Ctrl+Backspace which deletes the entire preceding word/letter (e.g. n in nên, m in một)!
            sleep_us(18000);
        }

        if (!str.empty()) {
            const char* ptr = str.data();
            const char* end = ptr + str.size();
            while (ptr < end) {
                uint32_t cp = utf8_next_codepoint(ptr, end);
                if (cp == 0) break;
                if (cp < 128) {
                    bool shift_needed = false;
                    int code = ascii_to_scancode(static_cast<char>(cp), shift_needed);
                    if (code > 0) {
                        if (shift_needed) {
                            emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                            sync();
                            sleep_us(step_delay);
                        }
                        tap_key(code, 2500);
                        if (shift_needed) {
                            emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                            sync();
                            sleep_us(step_delay);
                        }
                    }
                } else {
                    emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                    emit_event(EV_KEY, KEY_LEFTCTRL, 1);
                    sync();
                    sleep_us(2500);

                    tap_key(KEY_U, 2000);

                    emit_event(EV_KEY, KEY_LEFTCTRL, 0);
                    emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                    sync();
                    sleep_us(2000);

                    char hex_buf[16];
                    std::snprintf(hex_buf, sizeof(hex_buf), "%x", cp);
                    for (int i = 0; hex_buf[i] != '\0'; ++i) {
                        int sc = hex_char_to_scancode(hex_buf[i]);
                        if (sc > 0) {
                            tap_key(sc, step_delay);
                        }
                    }

                    // Commit via Space: never triggers form or chat send
                    tap_key(KEY_SPACE, 1500);

                    // Allow target application to process commit and close preedit
                    // before injecting subsequent characters in this replacement string (2 full 60Hz frames: ~32ms)
                    sleep_us(32000);
                }
                sleep_us(step_delay);
            }
        }

        restore_all_modifiers(mod);
    }
};

} // namespace senkey
