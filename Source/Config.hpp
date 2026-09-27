// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include "Types.hpp"
#include <string>

namespace senkey {

class ConfigManager {
private:
    std::string config_path;
    SenKeyConfig config;

    std::string get_default_config_path();

public:
    ConfigManager();
    explicit ConfigManager(const std::string& custom_path);

    bool load();
    bool save();

    const SenKeyConfig& get() const { return config; }
    SenKeyConfig& get_mutable() { return config; }

    void set_input_method(InputMethod im) { config.input_method = im; }
    void set_hotkey(HotkeyToggle hk) { config.hotkey = hk; }
};

} // namespace senkey
