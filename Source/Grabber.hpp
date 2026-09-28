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
#include <string>
#include <memory>
#include "Logger.hpp"

namespace senkey {

struct GrabbedDevice {
    int fd = -1;
    std::string path;
    std::string name;

    ~GrabbedDevice() {
        if (fd >= 0) {
            ioctl(fd, EVIOCGRAB, 0);
            close(fd);
            fd = -1;
        }
    }
};

class KeyboardGrabber {
private:
    std::vector<std::shared_ptr<GrabbedDevice>> devices;
    int epoll_fd = -1;

    static bool test_bit(int nr, const uint8_t* addr) {
        return (addr[nr / 8] & (1 << (nr % 8))) != 0;
    }

    bool is_real_keyboard(int fd, std::string& out_name) {
        char name[256] = "Unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        out_name = name;

        if (out_name.find("senkey") != std::string::npos ||
            out_name.find("vtux") != std::string::npos ||
            out_name.find("uinput") != std::string::npos) {
            return false;
        }

        uint8_t ev_bits[(EV_MAX + 7) / 8] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
            return false;
        }

        if (!test_bit(EV_KEY, ev_bits)) {
            return false;
        }

        uint8_t key_bits[(KEY_MAX + 7) / 8] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
            return false;
        }

        return test_bit(KEY_A, key_bits) &&
               test_bit(KEY_Z, key_bits) &&
               test_bit(KEY_SPACE, key_bits) &&
               test_bit(KEY_ENTER, key_bits);
    }

public:
    KeyboardGrabber() = default;

    ~KeyboardGrabber() {
        close_all();
    }

    KeyboardGrabber(const KeyboardGrabber&) = delete;
    KeyboardGrabber& operator=(const KeyboardGrabber&) = delete;

    bool init_and_grab_all() {
        close_all();

        epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) {
            return false;
        }

        DIR* dir = opendir("/dev/input");
        if (!dir) {
            return false;
        }

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (std::strncmp(ent->d_name, "event", 5) != 0) {
                continue;
            }

            std::string path = std::string("/dev/input/") + ent->d_name;
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            std::string name;
            if (is_real_keyboard(fd, name)) {
                if (ioctl(fd, EVIOCGRAB, 1) == 0) {
                    auto dev = std::make_shared<GrabbedDevice>();
                    dev->fd = fd;
                    dev->path = path;
                    dev->name = name;

                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.ptr = dev.get();
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == 0) {
                        devices.push_back(dev);
                        Logger::info("Grabbed keyboard: " + name + " (" + path + ")");
                    } else {
                        ioctl(fd, EVIOCGRAB, 0);
                        close(fd);
                    }
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }

        closedir(dir);
        return !devices.empty();
    }

    void close_all() {
        devices.clear();
        if (epoll_fd >= 0) {
            close(epoll_fd);
            epoll_fd = -1;
        }
    }

    int wait_events(std::vector<input_event>& out_events, int timeout_ms = 50) {
        out_events.clear();
        if (epoll_fd < 0 || devices.empty()) return -1;

        struct epoll_event ep_events[16];
        int nfds = epoll_wait(epoll_fd, ep_events, 16, timeout_ms);
        if (nfds <= 0) return nfds;

        for (int i = 0; i < nfds; ++i) {
            auto* dev = static_cast<GrabbedDevice*>(ep_events[i].data.ptr);
            if (!dev || dev->fd < 0) continue;

            struct input_event evs[32];
            ssize_t bytes = read(dev->fd, evs, sizeof(evs));
            if (bytes > 0) {
                int count = bytes / sizeof(struct input_event);
                for (int j = 0; j < count; ++j) {
                    out_events.push_back(evs[j]);
                }
            }
        }

        return static_cast<int>(out_events.size());
    }

    size_t grabbed_count() const {
        return devices.size();
    }

    bool is_capslock_on() const {
        for (const auto& dev : devices) {
            if (dev && dev->fd >= 0) {
                uint8_t leds[(LED_MAX + 7) / 8] = {0};
                if (ioctl(dev->fd, EVIOCGLED(sizeof(leds)), leds) >= 0) {
                    if (test_bit(LED_CAPSL, leds)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

} // namespace senkey
