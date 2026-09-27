// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace senkey {

class IpcServer {
private:
    int server_fd = -1;
    std::string socket_path;
    std::thread worker_thread;
    std::atomic<bool> running{false};

    std::function<std::string(const std::string&)> command_handler;

    std::string get_default_socket_path();
    void thread_loop();

public:
    IpcServer() = default;
    ~IpcServer();

    bool start(std::function<std::string(const std::string&)> handler);
    void stop();

    static bool send_command(const std::string& cmd, std::string& out_response);
};

} // namespace senkey
