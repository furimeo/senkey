// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <string>
#include "Types.hpp"
#include "ukengine.h"
#include "Macro.hpp"

namespace senkey {

class EngineWrapper {
private:
    UkEngine engine;
    UkSharedMem shared_mem;
    MacroManager macro_mgr;
    std::string current_raw_word;
    bool macro_enabled = true;

public:
    EngineWrapper();
    ~EngineWrapper() = default;

    void apply_config(const SenKeyConfig& config);
    void load_macros(const std::string& custom_path = "");

    bool process_key(char c, int& out_backs, std::string& out_str);
    bool process_backspace(int& out_backs, std::string& out_str);
    void reset();
};

} // namespace senkey
