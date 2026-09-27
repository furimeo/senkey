// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <cstring>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

namespace senkey {

class MouseWatcher {
private:
    std::vector<int> mouse_fds;
    int epoll_fd = -1;
    std::thread worker_thread;
    std::atomic<bool> running{false};
    std::function<void()> on_click_callback;

    static bool test_bit(int nr, const uint8_t* addr) {
        return (addr[nr / 8] & (1 << (nr % 8))) != 0;
    }

    bool is_mouse(int fd) {
        uint8_t ev_bits[(EV_MAX + 7) / 8] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
            return false;
        }

        if (!test_bit(EV_KEY, ev_bits) || !test_bit(EV_REL, ev_bits)) {
            return false;
        }

        uint8_t key_bits[(KEY_MAX + 7) / 8] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
            return false;
        }

        return test_bit(BTN_LEFT, key_bits);
    }

    void thread_loop() {
        struct epoll_event events[16];
        while (running) {
            int nfds = epoll_wait(epoll_fd, events, 16, 200);
            if (nfds <= 0) continue;

            for (int i = 0; i < nfds; ++i) {
                int fd = events[i].data.fd;
                struct input_event evs[16];
                ssize_t bytes = read(fd, evs, sizeof(evs));
                if (bytes > 0) {
                    int count = bytes / sizeof(struct input_event);
                    for (int j = 0; j < count; ++j) {
                        if (evs[j].type == EV_KEY &&
                            (evs[j].code == BTN_LEFT || evs[j].code == BTN_RIGHT) &&
                            evs[j].value == 1) {
                            if (on_click_callback) {
                                on_click_callback();
                            }
                        }
                    }
                }
            }
        }
    }

public:
    MouseWatcher() = default;

    ~MouseWatcher() {
        stop();
    }

    void start(std::function<void()> on_click) {
        stop();
        on_click_callback = on_click;

        epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) return;

        DIR* dir = opendir("/dev/input");
        if (!dir) {
            close(epoll_fd);
            epoll_fd = -1;
            return;
        }

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (std::strncmp(ent->d_name, "event", 5) != 0) continue;

            std::string path = std::string("/dev/input/") + ent->d_name;
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            if (is_mouse(fd)) {
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == 0) {
                    mouse_fds.push_back(fd);
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }
        closedir(dir);

        if (!mouse_fds.empty()) {
            running = true;
            worker_thread = std::thread(&MouseWatcher::thread_loop, this);
        }
    }

    void stop() {
        running = false;
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
        for (int fd : mouse_fds) {
            close(fd);
        }
        mouse_fds.clear();
        if (epoll_fd >= 0) {
            close(epoll_fd);
            epoll_fd = -1;
        }
    }
};

} // namespace senkey
