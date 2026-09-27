// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace senkey {

LogLevel Logger::current_level = LogLevel::INFO;
std::mutex Logger::log_mutex;

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex);
    current_level = level;
}

LogLevel Logger::get_level() {
    return current_level;
}

static void print_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&t, &tm);
    std::cout << "[" << std::setfill('0')
              << std::setw(2) << tm.tm_hour << ":"
              << std::setw(2) << tm.tm_min << ":"
              << std::setw(2) << tm.tm_sec << "] ";
}

void Logger::debug(const std::string& msg) {
    if (current_level > LogLevel::DEBUG) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    print_time();
    std::cout << "\033[90m[DEBUG]\033[0m " << msg << "\n" << std::flush;
}

void Logger::info(const std::string& msg) {
    if (current_level > LogLevel::INFO) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    print_time();
    std::cout << "\033[32m[INFO]\033[0m  " << msg << "\n" << std::flush;
}

void Logger::warn(const std::string& msg) {
    if (current_level > LogLevel::WARN) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    print_time();
    std::cout << "\033[33m[WARN]\033[0m  " << msg << "\n" << std::flush;
}

void Logger::error(const std::string& msg) {
    if (current_level > LogLevel::ERROR) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    print_time();
    std::cerr << "\033[31m[ERROR]\033[0m " << msg << "\n" << std::flush;
}

} // namespace senkey
