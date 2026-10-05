// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <linux/input.h>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>

#if defined(HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#undef None
#undef Status
#undef Bool
#undef Success
#undef True
#undef False
#endif

namespace senkey {

struct ModifierState {
    bool lshift = false;
    bool rshift = false;
    bool lctrl = false;
    bool rctrl = false;
    bool lalt = false;
    bool ralt = false;
    bool super = false;
    bool capslock = false;

    bool any_shift() const { return lshift || rshift; }
    bool any_ctrl() const { return lctrl || rctrl; }
    bool any_alt() const { return lalt || ralt; }
    bool any_active() const { return any_shift() || any_ctrl() || any_alt() || super || capslock; }
};

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

struct XkbConfig {
    std::string layout = "us";
    std::string variant = "";
    std::string options = "";
    std::string model = "pc105";
};

inline XkbConfig detect_system_layout_info() {
    XkbConfig info;
    info.layout = "";

    // 1. Biến môi trường XKB_DEFAULT_* (Ưu tiên cao nhất từ môi trường chạy)
    const char* env_layout = std::getenv("XKB_DEFAULT_LAYOUT");
    const char* env_variant = std::getenv("XKB_DEFAULT_VARIANT");
    const char* env_options = std::getenv("XKB_DEFAULT_OPTIONS");
    const char* env_model = std::getenv("XKB_DEFAULT_MODEL");

    if (env_layout && env_layout[0] != '\0') {
        info.layout = env_layout;
        if (env_variant && env_variant[0] != '\0') info.variant = env_variant;
        if (env_options && env_options[0] != '\0') info.options = env_options;
        if (env_model && env_model[0] != '\0') info.model = env_model;
        return info;
    }

    // 2. Kiểm tra XKB desktop session hiện tại (X11 root window property _XKB_RULES_NAMES)
#if defined(HAVE_X11)
    const char* dpy_name = std::getenv("DISPLAY");
    if (dpy_name && dpy_name[0] != '\0') {
        Display* dpy = XOpenDisplay(nullptr);
        if (dpy) {
            Atom prop = XInternAtom(dpy, "_XKB_RULES_NAMES", 1);
            if (prop != 0) {
                Atom actual_type = 0;
                int actual_format = 0;
                unsigned long nitems = 0, bytes_after = 0;
                unsigned char* prop_data = nullptr;
                if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), prop, 0, 1024,
                                       0, XA_STRING, &actual_type, &actual_format,
                                       &nitems, &bytes_after, &prop_data) == 0 && prop_data) {
                    const char* p = reinterpret_cast<const char*>(prop_data);
                    const char* end = p + nitems;
                    std::vector<std::string> parts;
                    while (p < end) {
                        parts.emplace_back(p);
                        p += parts.back().size() + 1;
                    }
                    XFree(prop_data);
                    // Format: rules\0model\0layout\0variant\0options\0
                    if (parts.size() >= 3 && !parts[2].empty()) {
                        info.layout = parts[2];
                        if (parts.size() >= 4) info.variant = parts[3];
                        if (parts.size() >= 5) info.options = parts[4];
                        if (parts.size() >= 2 && !parts[1].empty()) info.model = parts[1];
                        XCloseDisplay(dpy);
                        return info;
                    }
                }
            }
            XCloseDisplay(dpy);
        }
    }
#endif

    auto strip_val = [](std::string s) -> std::string {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(0, 1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            s = s.substr(1, s.size() - 2);
        }
        return s;
    };

    // 3. Kiểm tra cấu hình bàn phím hệ thống Debian/Ubuntu/Mint (/etc/default/keyboard)
    std::ifstream kb_file("/etc/default/keyboard");
    if (kb_file.is_open()) {
        std::string line;
        while (std::getline(kb_file, line)) {
            if (line.rfind("XKBLAYOUT=", 0) == 0) {
                std::string val = strip_val(line.substr(10));
                if (!val.empty()) info.layout = val;
            } else if (line.rfind("XKBVARIANT=", 0) == 0) {
                std::string val = strip_val(line.substr(11));
                if (!val.empty()) info.variant = val;
            } else if (line.rfind("XKBOPTIONS=", 0) == 0) {
                std::string val = strip_val(line.substr(11));
                if (!val.empty()) info.options = val;
            } else if (line.rfind("XKBMODEL=", 0) == 0) {
                std::string val = strip_val(line.substr(9));
                if (!val.empty()) info.model = val;
            }
        }
        if (!info.layout.empty()) return info;
    }

    // 4. Kiểm tra cấu hình bàn phím hệ thống Arch/Fedora (/etc/vconsole.conf)
    // CHÚ Ý: CHỈ đọc XKBLAYOUT=, XKBVARIANT=, XKBOPTIONS=, XKBMODEL=.
    // TUYỆT ĐỐI KHÔNG đọc KEYMAP= vì KEYMAP dùng cho Linux console kbd, không phải XKB layout name!
    std::ifstream vc_file("/etc/vconsole.conf");
    if (vc_file.is_open()) {
        std::string line;
        while (std::getline(vc_file, line)) {
            if (line.rfind("XKBLAYOUT=", 0) == 0) {
                std::string val = strip_val(line.substr(10));
                if (!val.empty()) info.layout = val;
            } else if (line.rfind("XKBVARIANT=", 0) == 0) {
                std::string val = strip_val(line.substr(11));
                if (!val.empty()) info.variant = val;
            } else if (line.rfind("XKBOPTIONS=", 0) == 0) {
                std::string val = strip_val(line.substr(11));
                if (!val.empty()) info.options = val;
            } else if (line.rfind("XKBMODEL=", 0) == 0) {
                std::string val = strip_val(line.substr(9));
                if (!val.empty()) info.model = val;
            }
        }
        if (!info.layout.empty()) return info;
    }

    // 5. Kiểm tra cấu hình bàn phím hệ thống Arch/Fedora (/etc/X11/xorg.conf.d/00-keyboard.conf do localectl sinh ra)
    std::ifstream xorg_file("/etc/X11/xorg.conf.d/00-keyboard.conf");
    if (xorg_file.is_open()) {
        std::string line;
        while (std::getline(xorg_file, line)) {
            auto pos_l = line.find("\"XkbLayout\"");
            if (pos_l != std::string::npos) {
                auto q1 = line.find('"', pos_l + 11);
                if (q1 != std::string::npos) {
                    auto q2 = line.find('"', q1 + 1);
                    if (q2 != std::string::npos) {
                        info.layout = line.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }
            auto pos_v = line.find("\"XkbVariant\"");
            if (pos_v != std::string::npos) {
                auto q1 = line.find('"', pos_v + 12);
                if (q1 != std::string::npos) {
                    auto q2 = line.find('"', q1 + 1);
                    if (q2 != std::string::npos) {
                        info.variant = line.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }
            auto pos_o = line.find("\"XkbOptions\"");
            if (pos_o != std::string::npos) {
                auto q1 = line.find('"', pos_o + 12);
                if (q1 != std::string::npos) {
                    auto q2 = line.find('"', q1 + 1);
                    if (q2 != std::string::npos) {
                        info.options = line.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }
        }
        if (!info.layout.empty()) return info;
    }

    // 6. Fallback mặc định
    info.layout = "us";
    return info;
}

inline std::string detect_system_layout() {
    return detect_system_layout_info().layout;
}

inline std::string build_symbols_include(const std::string& layout_str, const std::string& variant_str = "") {
    if (layout_str.empty()) {
        return "pc+us+inet(evdev)";
    }
    if (layout_str.find('+') != std::string::npos) {
        return layout_str;
    }

    std::vector<std::string> layouts;
    {
        std::stringstream ss(layout_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) layouts.push_back(item);
        }
    }
    if (layouts.empty()) layouts.push_back("us");

    std::vector<std::string> variants;
    {
        std::stringstream ss(variant_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            variants.push_back(item);
        }
    }

    std::string inc = "pc+";
    for (size_t i = 0; i < layouts.size(); ++i) {
        std::string var = (i < variants.size()) ? variants[i] : "";
        std::string part = layouts[i];
        if (!var.empty()) {
            part += "(" + var + ")";
        }
        if (i == 0) {
            inc += part + "+inet(evdev)";
        } else {
            inc += "+" + part + ":" + std::to_string(i + 1);
        }
    }
    return inc;
}

} // namespace senkey
