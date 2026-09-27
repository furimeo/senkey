// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <string>
#include <mutex>

namespace senkey {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    OFF = 4
};

class Logger {
private:
    static LogLevel current_level;
    static std::mutex log_mutex;

public:
    static void set_level(LogLevel level);
    static LogLevel get_level();

    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
};

} // namespace senkey
