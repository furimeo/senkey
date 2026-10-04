// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Config.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>

namespace senkey {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string ConfigManager::get_default_config_path() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::string base;
    if (xdg && xdg[0] != '\0') {
        base = xdg;
    } else {
        const char* home = std::getenv("HOME");
        base = home ? std::string(home) + "/.config" : "/tmp";
    }
    std::string dir = base + "/senkey";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir + "/senkey.conf";
}

ConfigManager::ConfigManager() {
    config_path = get_default_config_path();
}

ConfigManager::ConfigManager(const std::string& custom_path) {
    config_path = custom_path.empty() ? get_default_config_path() : custom_path;
}

bool ConfigManager::load() {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        Logger::info("Config not found at " + config_path + ". Writing defaults.");
        return save();
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (key == "input_method") {
            if (val == "vni") config.input_method = InputMethod::VNI;
            else if (val == "simple_telex") config.input_method = InputMethod::SIMPLE_TELEX;
            else config.input_method = InputMethod::TELEX;
        } else if (key == "hotkey") {
            if (val == "alt_z") config.hotkey = HotkeyToggle::ALT_Z;
            else if (val == "super_space") config.hotkey = HotkeyToggle::SUPER_SPACE;
            else config.hotkey = HotkeyToggle::CTRL_SHIFT;
        } else if (key == "modern_spelling") {
            config.modern_spelling = (val == "1" || val == "true" || val == "yes");
        } else if (key == "free_marking") {
            config.free_marking = (val == "1" || val == "true" || val == "yes");
        } else if (key == "spell_check") {
            config.spell_check = (val == "1" || val == "true" || val == "yes");
        } else if (key == "macro_enabled") {
            config.macro_enabled = (val == "1" || val == "true" || val == "yes");
        } else if (key == "show_tray") {
            config.show_tray = (val == "1" || val == "true" || val == "yes");
        } else if (key == "micro_delay_us") {
            int d = std::atoi(val.c_str());
            if (d >= 800 && d <= 10000) {
                config.micro_delay_us = d;
            } else {
                config.micro_delay_us = 1200;
            }
        }
    }
    if (config.micro_delay_us < 800) {
        config.micro_delay_us = 1200;
    }
    return true;
}

bool ConfigManager::save() {
    std::ofstream file(config_path);
    if (!file.is_open()) {
        Logger::error("Failed to write config to " + config_path);
        return false;
    }

    file << "# senkey configuration\n\n";
    file << "input_method=" << (config.input_method == InputMethod::VNI ? "vni" : "telex") << "\n";
    file << "hotkey=" << (config.hotkey == HotkeyToggle::ALT_Z ? "alt_z" : "ctrl_shift") << "\n";
    file << "modern_spelling=" << (config.modern_spelling ? "true" : "false") << "\n";
    file << "free_marking=" << (config.free_marking ? "true" : "false") << "\n";
    file << "spell_check=" << (config.spell_check ? "true" : "false") << "\n";
    file << "macro_enabled=" << (config.macro_enabled ? "true" : "false") << "\n";
    file << "show_tray=" << (config.show_tray ? "true" : "false") << "\n";
    file << "micro_delay_us=" << config.micro_delay_us << "\n";

    return true;
}

} // namespace senkey
