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
#include "Output/OutputBackend.hpp"

namespace senkey {

class VirtualKeyboard : public OutputBackend {
private:
    int uinput_fd = -1;

    void sleep_us(int us) {
        if (us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(us));
        }
    }

public:
    VirtualKeyboard() = default;

    ~VirtualKeyboard() override {
        close_device();
    }

    VirtualKeyboard(const VirtualKeyboard&) = delete;
    VirtualKeyboard& operator=(const VirtualKeyboard&) = delete;

    bool open_device(const char* device_name = "senkey-keyboard") override {
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

    void close_device() override {
        if (uinput_fd >= 0) {
            ioctl(uinput_fd, UI_DEV_DESTROY);
            close(uinput_fd);
            uinput_fd = -1;
        }
    }

    bool is_valid() const override {
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

    void emit_passthrough(const struct input_event& ev) override {
        if (uinput_fd < 0) return;
        while (write(uinput_fd, &ev, sizeof(ev)) < 0) {
            if (errno == EINTR) continue;
            break;
        }
    }

    void passthrough(const struct input_event& ev) {
        emit_passthrough(ev);
    }

    void tap_key(int code, int delay_us = 400) {
        if (uinput_fd < 0 || code <= 0) return;
        emit_event(EV_KEY, code, 1);
        sync();
        sleep_us(delay_us);
        emit_event(EV_KEY, code, 0);
        sync();
        sleep_us(delay_us);
    }

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
            tap_key(KEY_CAPSLOCK, 150);
            changed = true;
        }
        if (changed) {
            sync();
        }
    }

    void restore_all_modifiers(const ModifierState& mod) {
        bool changed = false;
        if (mod.capslock) {
            tap_key(KEY_CAPSLOCK, 150);
            changed = true;
        }
        if (mod.lshift) { emit_event(EV_KEY, KEY_LEFTSHIFT, 1); changed = true; }
        if (mod.rshift) { emit_event(EV_KEY, KEY_RIGHTSHIFT, 1); changed = true; }
        if (mod.lctrl)  { emit_event(EV_KEY, KEY_LEFTCTRL, 1); changed = true; }
        if (mod.rctrl)  { emit_event(EV_KEY, KEY_RIGHTCTRL, 1); changed = true; }
        if (mod.lalt)   { emit_event(EV_KEY, KEY_LEFTALT, 1); changed = true; }
        if (mod.ralt)   { emit_event(EV_KEY, KEY_RIGHTALT, 1); changed = true; }
        if (mod.super)  {
            emit_event(EV_KEY, KEY_LEFTMETA, 1);
            changed = true;
        }
        if (changed) {
            sync();
        }
    }

    // High performance batched emission
    void emit_replacement(int backs, const std::string& str, const ModifierState& mod = {}) override {
        if (backs <= 0 && str.empty()) return;

        release_all_modifiers(mod);

        // 1. Batched backspaces: down and up without sleeping, single sync
        if (backs > 0) {
            for (int i = 0; i < backs; ++i) {
                emit_event(EV_KEY, KEY_BACKSPACE, 1);
                emit_event(EV_KEY, KEY_BACKSPACE, 0);
            }
            sync();
        }

        if (str.empty()) {
            restore_all_modifiers(mod);
            return;
        }

        // 2. Check if replacement is pure ASCII
        bool all_ascii = true;
        for (unsigned char c : str) {
            if (c >= 128) {
                all_ascii = false;
                break;
            }
        }

        if (all_ascii) {
            // Pure ASCII: zero sleep, batch write!
            for (char c : str) {
                bool shift_needed = false;
                int code = ascii_to_scancode(c, shift_needed);
                if (code > 0) {
                    if (shift_needed) emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                    emit_event(EV_KEY, code, 1);
                    emit_event(EV_KEY, code, 0);
                    if (shift_needed) emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                }
            }
            sync();
            restore_all_modifiers(mod);
            return;
        }

        // 3. Unicode characters: fast low-latency fallback
        const char* ptr = str.data();
        const char* end = ptr + str.size();
        while (ptr < end) {
            uint32_t cp = utf8_next_codepoint(ptr, end);
            if (cp == 0) break;
            if (cp < 128) {
                bool shift_needed = false;
                int code = ascii_to_scancode(static_cast<char>(cp), shift_needed);
                if (code > 0) {
                    if (shift_needed) emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                    emit_event(EV_KEY, code, 1);
                    emit_event(EV_KEY, code, 0);
                    if (shift_needed) emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                    sync();
                }
            } else {
                emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                emit_event(EV_KEY, KEY_LEFTCTRL, 1);
                sync();
                sleep_us(400);

                emit_event(EV_KEY, KEY_U, 1);
                emit_event(EV_KEY, KEY_U, 0);
                sync();
                sleep_us(400);

                emit_event(EV_KEY, KEY_LEFTCTRL, 0);
                emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                sync();
                sleep_us(400);

                char hex_buf[16];
                std::snprintf(hex_buf, sizeof(hex_buf), "%x", cp);
                for (int i = 0; hex_buf[i] != '\0'; ++i) {
                    int sc = hex_char_to_scancode(hex_buf[i]);
                    if (sc > 0) {
                        emit_event(EV_KEY, sc, 1);
                        emit_event(EV_KEY, sc, 0);
                    }
                }
                sync();
                sleep_us(400);

                emit_event(EV_KEY, KEY_SPACE, 1);
                emit_event(EV_KEY, KEY_SPACE, 0);
                sync();
                sleep_us(1000);
            }
        }

        restore_all_modifiers(mod);
    }
};

using UInputBackend = VirtualKeyboard;

} // namespace senkey
