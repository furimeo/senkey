// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <linux/input.h>
#include <cstdint>

namespace senkey {

inline bool is_modifier(int code) {
    switch (code) {
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL:
        case KEY_LEFTALT:
        case KEY_RIGHTALT:
        case KEY_LEFTMETA:
        case KEY_RIGHTMETA:
        case KEY_CAPSLOCK:
            return true;
        default:
            return false;
    }
}

inline bool is_navigation_or_reset(int code) {
    switch (code) {
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_UP:
        case KEY_DOWN:
        case KEY_HOME:
        case KEY_END:
        case KEY_PAGEUP:
        case KEY_PAGEDOWN:
        case KEY_ESC:
        case KEY_TAB:
        case KEY_ENTER:
        case KEY_KPENTER:
            return true;
        default:
            return false;
    }
}

inline char scancode_to_ascii(int code, bool shift, bool capslock) {
    bool upper = shift ^ capslock;
    switch (code) {
        case KEY_A: return upper ? 'A' : 'a';
        case KEY_B: return upper ? 'B' : 'b';
        case KEY_C: return upper ? 'C' : 'c';
        case KEY_D: return upper ? 'D' : 'd';
        case KEY_E: return upper ? 'E' : 'e';
        case KEY_F: return upper ? 'F' : 'f';
        case KEY_G: return upper ? 'G' : 'g';
        case KEY_H: return upper ? 'H' : 'h';
        case KEY_I: return upper ? 'I' : 'i';
        case KEY_J: return upper ? 'J' : 'j';
        case KEY_K: return upper ? 'K' : 'k';
        case KEY_L: return upper ? 'L' : 'l';
        case KEY_M: return upper ? 'M' : 'm';
        case KEY_N: return upper ? 'N' : 'n';
        case KEY_O: return upper ? 'O' : 'o';
        case KEY_P: return upper ? 'P' : 'p';
        case KEY_Q: return upper ? 'Q' : 'q';
        case KEY_R: return upper ? 'R' : 'r';
        case KEY_S: return upper ? 'S' : 's';
        case KEY_T: return upper ? 'T' : 't';
        case KEY_U: return upper ? 'U' : 'u';
        case KEY_V: return upper ? 'V' : 'v';
        case KEY_W: return upper ? 'W' : 'w';
        case KEY_X: return upper ? 'X' : 'x';
        case KEY_Y: return upper ? 'Y' : 'y';
        case KEY_Z: return upper ? 'Z' : 'z';

        case KEY_1: return shift ? '!' : '1';
        case KEY_2: return shift ? '@' : '2';
        case KEY_3: return shift ? '#' : '3';
        case KEY_4: return shift ? '$' : '4';
        case KEY_5: return shift ? '%' : '5';
        case KEY_6: return shift ? '^' : '6';
        case KEY_7: return shift ? '&' : '7';
        case KEY_8: return shift ? '*' : '8';
        case KEY_9: return shift ? '(' : '9';
        case KEY_0: return shift ? ')' : '0';

        case KEY_MINUS: return shift ? '_' : '-';
        case KEY_EQUAL: return shift ? '+' : '=';
        case KEY_LEFTBRACE: return shift ? '{' : '[';
        case KEY_RIGHTBRACE: return shift ? '}' : ']';
        case KEY_BACKSLASH: return shift ? '|' : '\\';
        case KEY_SEMICOLON: return shift ? ':' : ';';
        case KEY_APOSTROPHE: return shift ? '"' : '\'';
        case KEY_GRAVE: return shift ? '~' : '`';
        case KEY_COMMA: return shift ? '<' : ',';
        case KEY_DOT: return shift ? '>' : '.';
        case KEY_SLASH: return shift ? '?' : '/';
        case KEY_SPACE: return ' ';

        default:
            return 0;
    }
}

inline int hex_char_to_scancode(char c) {
    if (c >= '0' && c <= '9') {
        switch (c) {
            case '0': return KEY_0;
            case '1': return KEY_1;
            case '2': return KEY_2;
            case '3': return KEY_3;
            case '4': return KEY_4;
            case '5': return KEY_5;
            case '6': return KEY_6;
            case '7': return KEY_7;
            case '8': return KEY_8;
            case '9': return KEY_9;
        }
    }
    if (c >= 'a' && c <= 'f') {
        switch (c) {
            case 'a': return KEY_A;
            case 'b': return KEY_B;
            case 'c': return KEY_C;
            case 'd': return KEY_D;
            case 'e': return KEY_E;
            case 'f': return KEY_F;
        }
    }
    if (c >= 'A' && c <= 'F') {
        switch (c) {
            case 'A': return KEY_A;
            case 'B': return KEY_B;
            case 'C': return KEY_C;
            case 'D': return KEY_D;
            case 'E': return KEY_E;
            case 'F': return KEY_F;
        }
    }
    return 0;
}

inline uint32_t utf8_next_codepoint(const char*& ptr, const char* end) {
    if (ptr >= end) return 0;
    uint8_t c = static_cast<uint8_t>(*ptr++);
    if (c < 0x80) {
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        if (ptr >= end) return 0;
        uint32_t cp = (c & 0x1F) << 6;
        cp |= (*ptr++ & 0x3F);
        return cp;
    } else if ((c & 0xF0) == 0xE0) {
        if (ptr + 1 >= end) return 0;
        uint32_t cp = (c & 0x0F) << 12;
        cp |= (*ptr++ & 0x3F) << 6;
        cp |= (*ptr++ & 0x3F);
        return cp;
    } else if ((c & 0xF8) == 0xF0) {
        if (ptr + 2 >= end) return 0;
        uint32_t cp = (c & 0x07) << 18;
        cp |= (*ptr++ & 0x3F) << 12;
        cp |= (*ptr++ & 0x3F) << 6;
        cp |= (*ptr++ & 0x3F);
        return cp;
    }
    return 0;
}

} // namespace senkey
