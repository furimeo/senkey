// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Macro.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace senkey {

std::string MacroManager::get_default_macro_path() {
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
    return dir + "/macro.txt";
}

MacroManager::MacroManager() {
    macro_path = get_default_macro_path();
}

MacroManager::MacroManager(const std::string& custom_path) {
    macro_path = custom_path.empty() ? get_default_macro_path() : custom_path;
}

bool MacroManager::load() {
    macros.clear();
    std::ifstream file(macro_path);
    if (!file.is_open()) {
        add("vn", "Việt Nam");
        add("hn", "Hà Nội");
        add("dc", "được");
        save();
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t colon = line.find(':');
        if (colon != std::string::npos && colon > 0) {
            macros[line.substr(0, colon)] = line.substr(colon + 1);
        }
    }
    return true;
}

bool MacroManager::save() {
    std::ofstream file(macro_path);
    if (!file.is_open()) {
        return false;
    }

    file << "# senkey macro definitions (key:replacement)\n\n";
    for (const auto& [k, v] : macros) {
        file << k << ":" << v << "\n";
    }
    return true;
}

void MacroManager::add(const std::string& key, const std::string& value) {
    macros[key] = value;
}

void MacroManager::remove(const std::string& key) {
    macros.erase(key);
}

std::string MacroManager::lookup(const std::string& key) const {
    auto it = macros.find(key);
    return it != macros.end() ? it->second : "";
}

} // namespace senkey
