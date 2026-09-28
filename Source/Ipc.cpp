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
    if (uid != 0) {
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        if (xdg && xdg[0] != '\0') {
            return std::string(xdg) + "/senkey.sock";
        }
    }
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

    // Bảo mật chuẩn Linux (Principle of Least Privilege):
    if (getuid() == 0) {
        // Daemon hệ thống: chỉ sở hữu bởi root:input với quyền 0660 (rw-rw----, other: ---).
        // Tuyệt đối không mở 0666 cho world.
        struct group* gr = getgrnam("input");
        if (gr) {
            chown(socket_path.c_str(), 0, gr->gr_gid);
            chmod(socket_path.c_str(), 0660);
        } else {
            chmod(socket_path.c_str(), 0600);
        }

        // Cấp quyền qua POSIX ACL đích danh cho tài khoản người dùng desktop (SENKEY_USER hoặc SUDO_USER)
        // để chỉ duy nhất người dùng đó được phép kết nối, chặn đứng hoàn toàn mọi truy cập trái phép khác.
        const char* target_user = std::getenv("SENKEY_USER");
        if (!target_user || target_user[0] == '\0') {
            target_user = std::getenv("SUDO_USER");
        }
        if (target_user && target_user[0] != '\0' && std::string(target_user) != "root") {
            std::string cmd = "setfacl -m u:" + std::string(target_user) + ":rw " + socket_path + " 2>/dev/null";
            system(cmd.c_str());
        }
    } else {
        // Daemon phiên người dùng: Chỉ duy nhất chủ sở hữu được truy cập (0600: rw-------).
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
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    if (xdg && xdg[0] != '\0') {
        paths.push_back(std::string(xdg) + "/senkey.sock");
    }
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
