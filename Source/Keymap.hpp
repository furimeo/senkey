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

inline int ascii_to_scancode(char c, bool& out_shift) {
    out_shift = false;
    switch (c) {
        case 'a': return KEY_A;
        case 'A': out_shift = true; return KEY_A;
        case 'b': return KEY_B;
        case 'B': out_shift = true; return KEY_B;
        case 'c': return KEY_C;
        case 'C': out_shift = true; return KEY_C;
        case 'd': return KEY_D;
        case 'D': out_shift = true; return KEY_D;
        case 'e': return KEY_E;
        case 'E': out_shift = true; return KEY_E;
        case 'f': return KEY_F;
        case 'F': out_shift = true; return KEY_F;
        case 'g': return KEY_G;
        case 'G': out_shift = true; return KEY_G;
        case 'h': return KEY_H;
        case 'H': out_shift = true; return KEY_H;
        case 'i': return KEY_I;
        case 'I': out_shift = true; return KEY_I;
        case 'j': return KEY_J;
        case 'J': out_shift = true; return KEY_J;
        case 'k': return KEY_K;
        case 'K': out_shift = true; return KEY_K;
        case 'l': return KEY_L;
        case 'L': out_shift = true; return KEY_L;
        case 'm': return KEY_M;
        case 'M': out_shift = true; return KEY_M;
        case 'n': return KEY_N;
        case 'N': out_shift = true; return KEY_N;
        case 'o': return KEY_O;
        case 'O': out_shift = true; return KEY_O;
        case 'p': return KEY_P;
        case 'P': out_shift = true; return KEY_P;
        case 'q': return KEY_Q;
        case 'Q': out_shift = true; return KEY_Q;
        case 'r': return KEY_R;
        case 'R': out_shift = true; return KEY_R;
        case 's': return KEY_S;
        case 'S': out_shift = true; return KEY_S;
        case 't': return KEY_T;
        case 'T': out_shift = true; return KEY_T;
        case 'u': return KEY_U;
        case 'U': out_shift = true; return KEY_U;
        case 'v': return KEY_V;
        case 'V': out_shift = true; return KEY_V;
        case 'w': return KEY_W;
        case 'W': out_shift = true; return KEY_W;
        case 'x': return KEY_X;
        case 'X': out_shift = true; return KEY_X;
        case 'y': return KEY_Y;
        case 'Y': out_shift = true; return KEY_Y;
        case 'z': return KEY_Z;
        case 'Z': out_shift = true; return KEY_Z;

        case '1': return KEY_1;
        case '!': out_shift = true; return KEY_1;
        case '2': return KEY_2;
        case '@': out_shift = true; return KEY_2;
        case '3': return KEY_3;
        case '#': out_shift = true; return KEY_3;
        case '4': return KEY_4;
        case '$': out_shift = true; return KEY_4;
        case '5': return KEY_5;
        case '%': out_shift = true; return KEY_5;
        case '6': return KEY_6;
        case '^': out_shift = true; return KEY_6;
        case '7': return KEY_7;
        case '&': out_shift = true; return KEY_7;
        case '8': return KEY_8;
        case '*': out_shift = true; return KEY_8;
        case '9': return KEY_9;
        case '(': out_shift = true; return KEY_9;
        case '0': return KEY_0;
        case ')': out_shift = true; return KEY_0;

        case '-': return KEY_MINUS;
        case '_': out_shift = true; return KEY_MINUS;
        case '=': return KEY_EQUAL;
        case '+': out_shift = true; return KEY_EQUAL;
        case '[': return KEY_LEFTBRACE;
        case '{': out_shift = true; return KEY_LEFTBRACE;
        case ']': return KEY_RIGHTBRACE;
        case '}': out_shift = true; return KEY_RIGHTBRACE;
        case '\\': return KEY_BACKSLASH;
        case '|': out_shift = true; return KEY_BACKSLASH;
        case ';': return KEY_SEMICOLON;
        case ':': out_shift = true; return KEY_SEMICOLON;
        case '\'': return KEY_APOSTROPHE;
        case '"': out_shift = true; return KEY_APOSTROPHE;
        case '`': return KEY_GRAVE;
        case '~': out_shift = true; return KEY_GRAVE;
        case ',': return KEY_COMMA;
        case '<': out_shift = true; return KEY_COMMA;
        case '.': return KEY_DOT;
        case '>': out_shift = true; return KEY_DOT;
        case '/': return KEY_SLASH;
        case '?': out_shift = true; return KEY_SLASH;
        case ' ': return KEY_SPACE;
        case '\t': return KEY_TAB;
        case '\n': return KEY_ENTER;
        default: return 0;
    }
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
