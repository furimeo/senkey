// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "Ipc.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <vector>
#include <grp.h>

namespace senkey {

std::string IpcServer::get_default_socket_path() {
    uid_t uid = getuid();
    return "/tmp/senkey-" + std::to_string(uid) + ".sock";
}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start(std::function<std::string(const std::string&)> handler) {
    stop();
    command_handler = handler;
    socket_path = get_default_socket_path();

    unlink(socket_path.c_str());

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    // Bảo mật Linux:
    // - Daemon người dùng thông thường: Chỉ chủ sở hữu có quyền (0600).
    // - Daemon hệ thống (root): Thuộc nhóm 'input' với quyền (0660), chỉ thành viên nhóm input mới gửi lệnh được.
    if (getuid() == 0) {
        struct group* gr = getgrnam("input");
        if (gr) {
            chown(socket_path.c_str(), 0, gr->gr_gid);
            chmod(socket_path.c_str(), 0660);
        } else {
            chmod(socket_path.c_str(), 0600);
        }
    } else {
        chmod(socket_path.c_str(), 0600);
    }

    if (listen(server_fd, 8) < 0) {
        close(server_fd);
        server_fd = -1;
        return false;
    }

    running = true;
    worker_thread = std::thread(&IpcServer::thread_loop, this);
    return true;
}

void IpcServer::stop() {
    running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    if (!socket_path.empty()) {
        unlink(socket_path.c_str());
        socket_path.clear();
    }
}

void IpcServer::thread_loop() {
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;

    while (running) {
        int ret = poll(&pfd, 1, 300);
        if (ret <= 0) continue;

        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;

        char buf[256] = {0};
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            std::string cmd(buf);
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
                cmd.pop_back();
            }

            std::string response = "OK\n";
            if (command_handler) {
                response = command_handler(cmd) + "\n";
            }
            write(client_fd, response.data(), response.size());
        }
        close(client_fd);
    }
}

bool IpcServer::send_command(const std::string& cmd, std::string& out_response) {
    uid_t uid = getuid();
    std::vector<std::string> paths;
    paths.push_back("/tmp/senkey-" + std::to_string(uid) + ".sock");
    if (uid != 0) {
        paths.push_back("/tmp/senkey-0.sock");
    }

    for (const auto& path : paths) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);
            continue;
        }

        std::string full_cmd = cmd + "\n";
        write(fd, full_cmd.data(), full_cmd.size());

        char buf[512] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            out_response = std::string(buf);
            while (!out_response.empty() && (out_response.back() == '\n' || out_response.back() == '\r')) {
                out_response.pop_back();
            }
        }
        close(fd);
        return true;
    }

    return false;
}

} // namespace senkey
