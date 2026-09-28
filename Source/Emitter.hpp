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

namespace senkey {

class VirtualKeyboard {
private:
    int uinput_fd = -1;

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

    bool emit_ascii_char(char c, int delay_us = 600, bool physical_shift = false) {
        bool shift_needed = false;
        int code = ascii_to_scancode(c, shift_needed);
        if (code <= 0) return false;

        bool toggle_shift = (physical_shift != shift_needed);
        if (toggle_shift) {
            emit_event(EV_KEY, KEY_LEFTSHIFT, shift_needed ? 1 : 0);
            sync();
            sleep_us(delay_us);
        }

        tap_key(code, delay_us);

        if (toggle_shift) {
            emit_event(EV_KEY, KEY_LEFTSHIFT, physical_shift ? 1 : 0);
            sync();
            sleep_us(delay_us);
        }

        return true;
    }

    void emit_unicode(uint32_t codepoint, int delay_us = 600, bool physical_shift = false, bool physical_ctrl = false) {
        if (codepoint < 128) {
            emit_ascii_char(static_cast<char>(codepoint), delay_us, physical_shift);
            return;
        }

        char hex_buf[16];
        std::snprintf(hex_buf, sizeof(hex_buf), "%x", codepoint);

        emit_event(EV_KEY, KEY_LEFTCTRL, 1);
        emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
        sync();
        sleep_us(delay_us);

        tap_key(KEY_U, delay_us);

        emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
        emit_event(EV_KEY, KEY_LEFTCTRL, 0);
        sync();
        sleep_us(delay_us);

        for (int i = 0; hex_buf[i] != '\0'; ++i) {
            int sc = hex_char_to_scancode(hex_buf[i]);
            if (sc > 0) {
                tap_key(sc, delay_us);
            }
        }

        tap_key(KEY_ENTER, delay_us);
        // Post-commit settling delay to give target GUI application event loops (e.g. Chrome, GTK)
        // enough time to process Enter, insert the unicode glyph, and close the preedit widget
        sleep_us(delay_us * 3);

        // Restore physical modifier states if needed
        if (physical_ctrl) emit_event(EV_KEY, KEY_LEFTCTRL, 1);
        if (physical_shift) emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
        if (physical_ctrl || physical_shift) {
            sync();
            sleep_us(delay_us);
        }
    }

    void emit_utf8_string(const std::string& str, int delay_us = 600, bool physical_shift = false, bool physical_ctrl = false) {
        const char* ptr = str.data();
        const char* end = ptr + str.size();
        while (ptr < end) {
            uint32_t cp = utf8_next_codepoint(ptr, end);
            if (cp == 0) break;
            emit_unicode(cp, delay_us, physical_shift, physical_ctrl);
            sleep_us(delay_us);
        }
    }
};

} // namespace senkey
