// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <cstdint>
#include <string>

namespace senkey {

enum class InputMethod : int {
    TELEX = 0,
    VNI = 1,
    SIMPLE_TELEX = 2
};

enum class Charset : int {
    UNICODE_UTF8 = 0,
    TCVN3 = 1,
    VNI_WINDOWS = 2
};

enum class HotkeyToggle : int {
    CTRL_SHIFT = 0,
    ALT_Z = 1,
    SUPER_SPACE = 2
};

enum class Mode : int {
    VIETNAMESE = 0,
    ENGLISH = 1
};

struct SenKeyConfig {
    InputMethod input_method = InputMethod::TELEX;
    Charset charset = Charset::UNICODE_UTF8;
    HotkeyToggle hotkey = HotkeyToggle::CTRL_SHIFT;
    bool free_marking = true;
    bool modern_spelling = true;
    bool spell_check = true;
    bool auto_non_vn_restore = true;
    bool macro_enabled = true;
    int micro_delay_us = 1200;
    std::string macro_file = "";
};

} // namespace senkey
