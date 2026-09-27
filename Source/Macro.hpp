// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <string>
#include <unordered_map>

namespace senkey {

class MacroManager {
private:
    std::unordered_map<std::string, std::string> macros;
    std::string macro_path;

    std::string get_default_macro_path();

public:
    MacroManager();
    explicit MacroManager(const std::string& custom_path);

    bool load();
    bool save();

    void add(const std::string& key, const std::string& value);
    void remove(const std::string& key);

    std::string lookup(const std::string& key) const;

    size_t size() const { return macros.size(); }
};

} // namespace senkey
