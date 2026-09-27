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
        uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (uinput_fd < 0) {
            uinput_fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);
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
        write(uinput_fd, &ev, sizeof(ev));
    }

    void sync() {
        emit_event(EV_SYN, SYN_REPORT, 0);
    }

    void passthrough(const struct input_event& ev) {
        if (uinput_fd < 0) return;
        write(uinput_fd, &ev, sizeof(ev));
    }

    void tap_key(int code, int delay_us = 1200) {
        if (uinput_fd < 0 || code <= 0) return;
        emit_event(EV_KEY, code, 1);
        sync();
        sleep_us(delay_us);
        emit_event(EV_KEY, code, 0);
        sync();
        sleep_us(delay_us);
    }

    void emit_backspaces(int count, int delay_us = 1200) {
        for (int i = 0; i < count; ++i) {
            tap_key(KEY_BACKSPACE, delay_us);
        }
    }

    bool emit_ascii_char(char c, int delay_us = 1200) {
        bool shift_needed = false;
        int code = 0;

        if (c >= 'a' && c <= 'z') {
            code = KEY_A + (c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            code = KEY_A + (c - 'A');
            shift_needed = true;
        } else if (c >= '1' && c <= '9') {
            code = KEY_1 + (c - '1');
        } else if (c == '0') {
            code = KEY_0;
        } else if (c == ' ') {
            code = KEY_SPACE;
        } else {
            switch (c) {
                case '!': code = KEY_1; shift_needed = true; break;
                case '@': code = KEY_2; shift_needed = true; break;
                case '#': code = KEY_3; shift_needed = true; break;
                case '$': code = KEY_4; shift_needed = true; break;
                case '%': code = KEY_5; shift_needed = true; break;
                case '^': code = KEY_6; shift_needed = true; break;
                case '&': code = KEY_7; shift_needed = true; break;
                case '*': code = KEY_8; shift_needed = true; break;
                case '(': code = KEY_9; shift_needed = true; break;
                case ')': code = KEY_0; shift_needed = true; break;
                case '-': code = KEY_MINUS; break;
                case '_': code = KEY_MINUS; shift_needed = true; break;
                case '=': code = KEY_EQUAL; break;
                case '+': code = KEY_EQUAL; shift_needed = true; break;
                case '[': code = KEY_LEFTBRACE; break;
                case '{': code = KEY_LEFTBRACE; shift_needed = true; break;
                case ']': code = KEY_RIGHTBRACE; break;
                case '}': code = KEY_RIGHTBRACE; shift_needed = true; break;
                case ';': code = KEY_SEMICOLON; break;
                case ':': code = KEY_SEMICOLON; shift_needed = true; break;
                case '\'': code = KEY_APOSTROPHE; break;
                case '"': code = KEY_APOSTROPHE; shift_needed = true; break;
                case ',': code = KEY_COMMA; break;
                case '<': code = KEY_COMMA; shift_needed = true; break;
                case '.': code = KEY_DOT; break;
                case '>': code = KEY_DOT; shift_needed = true; break;
                case '/': code = KEY_SLASH; break;
                case '?': code = KEY_SLASH; shift_needed = true; break;
                default: return false;
            }
        }

        if (shift_needed) {
            emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
            sync();
            sleep_us(delay_us);
        }

        tap_key(code, delay_us);

        if (shift_needed) {
            emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
            sync();
            sleep_us(delay_us);
        }

        return true;
    }

    void emit_unicode(uint32_t codepoint, int delay_us = 1000) {
        if (codepoint < 128) {
            emit_ascii_char(static_cast<char>(codepoint), delay_us);
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
    }

    void emit_utf8_string(const std::string& str, int delay_us = 1000) {
        const char* ptr = str.data();
        const char* end = ptr + str.size();
        while (ptr < end) {
            uint32_t cp = utf8_next_codepoint(ptr, end);
            if (cp == 0) break;
            emit_unicode(cp, delay_us);
        }
    }
};

} // namespace senkey
