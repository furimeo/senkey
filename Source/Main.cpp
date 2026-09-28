// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <unistd.h>

#include "Version.hpp"
#include "Types.hpp"
#include "Logger.hpp"
#include "Config.hpp"
#include "Keymap.hpp"
#include "Emitter.hpp"
#include "Grabber.hpp"
#include "Hotplug.hpp"
#include "MouseWatcher.hpp"
#include "EngineWrapper.hpp"
#include "Ipc.hpp"

using namespace senkey;

static std::atomic<bool> g_running{true};

static void handle_signal(int sig) {
    (void)sig;
    g_running = false;
}

static void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -g, --gui          Open graphical control panel\n"
              << "  -t, --toggle       Toggle active mode (V/E) of running background service\n"
              << "  -s, --status       Print current mode (V or E) of running background service\n"
              << "  -q, --quit         Terminate running background service\n"
              << "  -r, --reload       Reload configuration and macros\n"
              << "  -c, --config PATH  Specify custom config file path\n"
              << "  -m, --macro PATH   Specify custom macro file path\n"
              << "      --verbose      Enable verbose debug logging\n"
              << "      --service      Run foreground service loop (managed by systemd)\n"
              << "  -v, --version      Show version information\n"
              << "  -h, --help         Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool is_service_mode = false;
    bool verbose = false;
    std::string custom_config;
    std::string custom_macro;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "SenKey " << senkey::VERSION << "\n";
            return 0;
        } else if (arg == "-g" || arg == "--gui") {
            execlp("senkey-gui", "senkey-gui", nullptr);
            execlp("./senkey-gui", "./senkey-gui", nullptr);
            std::cerr << "senkey-gui executable not found\n";
            return 1;
        } else if (arg == "-t" || arg == "--toggle") {
            std::string resp;
            if (IpcServer::send_command("TOGGLE", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey background service is not running\n";
            return 1;
        } else if (arg == "-s" || arg == "--status") {
            std::string resp;
            if (IpcServer::send_command("STATUS", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey background service is not running\n";
            return 1;
        } else if (arg == "-q" || arg == "--quit") {
            std::string resp;
            if (IpcServer::send_command("QUIT", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey background service is not running\n";
            return 1;
        } else if (arg == "-r" || arg == "--reload") {
            std::string resp;
            if (IpcServer::send_command("RELOAD", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey background service is not running\n";
            return 1;
        } else if (arg == "--service") {
            is_service_mode = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            custom_config = argv[++i];
        } else if ((arg == "-m" || arg == "--macro") && i + 1 < argc) {
            custom_macro = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (verbose) {
        Logger::set_level(LogLevel::DEBUG);
    } else {
        Logger::set_level(LogLevel::INFO);
    }

    // Mô hình tất định: SenKey luôn chạy nền.
    // Nếu chạy không cờ (từ menu hoặc terminal) và không phải cờ --service của systemd:
    if (!is_service_mode) {
        std::string status_resp;
        if (IpcServer::send_command("STATUS", status_resp)) {
            // Dịch vụ nền đã chạy: hiển thị Bảng điều khiển GUI
            if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
                execlp("senkey-gui", "senkey-gui", nullptr);
                execlp("./senkey-gui", "./senkey-gui", nullptr);
            }
            std::cout << "SenKey is running in background (Mode: [" << status_resp << "]).\n";
            return 0;
        }

        // Tự động chuyển vào nền (daemonize) để giải phóng terminal
        if (daemon(0, 0) != 0) {
            Logger::error("Failed to run SenKey in background");
            return 1;
        }

        // Khởi động Bảng điều khiển và biểu tượng Khay hệ thống nếu có môi trường đồ họa
        if (std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY")) {
            pid_t gui_pid = fork();
            if (gui_pid == 0) {
                setsid();
                execlp("senkey-gui", "senkey-gui", nullptr);
                execlp("./senkey-gui", "./senkey-gui", nullptr);
                _exit(0);
            }
        }
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGHUP, handle_signal);

    ConfigManager cfg_mgr(custom_config);
    cfg_mgr.load();
    const SenKeyConfig& cfg = cfg_mgr.get();

    VirtualKeyboard emitter;
    if (!emitter.open_device("senkey-keyboard")) {
        std::string err_str = std::strerror(errno);
        Logger::error("Failed to open /dev/uinput: " + err_str + 
                      ". Ensure your user is in the 'input' group (try running 'newgrp input' or re-login), "
                      "or verify the uinput kernel module is loaded.");
        return 1;
    }

    KeyboardGrabber grabber;
    if (!grabber.init_and_grab_all()) {
        Logger::warn("No physical keyboards detected yet in /dev/input. SenKey is active and waiting for devices...");
    }

    EngineWrapper engine;
    engine.apply_config(cfg);
    engine.load_macros(custom_macro);

    HotplugWatcher hotplug;
    hotplug.start([&grabber]() {
        grabber.init_and_grab_all();
    });

    MouseWatcher mouse_watcher;
    mouse_watcher.start([&engine]() {
        engine.reset();
    });

    std::atomic<bool> vietnamese_enabled{true};

    IpcServer ipc;
    ipc.start([&](const std::string& cmd) -> std::string {
        if (cmd == "STATUS") {
            return vietnamese_enabled ? "V" : "E";
        } else if (cmd == "TOGGLE") {
            vietnamese_enabled = !vietnamese_enabled;
            engine.reset();
            return vietnamese_enabled ? "V" : "E";
        } else if (cmd == "QUIT") {
            g_running = false;
            return "OK";
        } else if (cmd == "RELOAD") {
            cfg_mgr.load();
            engine.apply_config(cfg_mgr.get());
            engine.load_macros();
            return "OK";
        }
        return "ERR";
    });

    bool lshift_down = false;
    bool rshift_down = false;
    bool lctrl_down = false;
    bool rctrl_down = false;
    bool lalt_down = false;
    bool ralt_down = false;
    bool super_down = false;
    bool capslock_on = false;

    std::vector<input_event> events;
    events.reserve(32);

    Logger::info("SenKey ready (" + std::string(vietnamese_enabled ? "V" : "E") + ")");

    while (g_running) {
        if (grabber.grabbed_count() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            static int retry_ticks = 0;
            if (++retry_ticks >= 8) {
                retry_ticks = 0;
                grabber.init_and_grab_all();
            }
            continue;
        }

        int count = grabber.wait_events(events, 50);
        if (count <= 0) continue;

        for (const auto& ev : events) {
            if (ev.type != EV_KEY) {
                emitter.passthrough(ev);
                continue;
            }

            int code = ev.code;
            int val = ev.value;

            if (code == KEY_LEFTSHIFT)  lshift_down = (val > 0);
            if (code == KEY_RIGHTSHIFT) rshift_down = (val > 0);
            if (code == KEY_LEFTCTRL)   lctrl_down = (val > 0);
            if (code == KEY_RIGHTCTRL)  rctrl_down = (val > 0);
            if (code == KEY_LEFTALT)    lalt_down = (val > 0);
            if (code == KEY_RIGHTALT)   ralt_down = (val > 0);
            if (code == KEY_LEFTMETA || code == KEY_RIGHTMETA) super_down = (val > 0);
            if (code == KEY_CAPSLOCK && val == 1) capslock_on = !capslock_on;

            bool shift = lshift_down || rshift_down;
            bool ctrl = lctrl_down || rctrl_down;
            bool alt = lalt_down || ralt_down;

            if (val == 1) {
                bool trigger = false;
                if (cfg.hotkey == HotkeyToggle::CTRL_SHIFT) {
                    trigger = (ctrl && (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT)) ||
                              (shift && (code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL));
                } else if (cfg.hotkey == HotkeyToggle::ALT_Z) {
                    trigger = (alt && code == KEY_Z);
                } else if (cfg.hotkey == HotkeyToggle::SUPER_SPACE) {
                    trigger = (super_down && code == KEY_SPACE);
                }

                if (trigger) {
                    vietnamese_enabled = !vietnamese_enabled;
                    engine.reset();
                    Logger::info(vietnamese_enabled ? "Mode: [V]" : "Mode: [E]");
                    continue;
                }
            }

            if (val == 0) {
                emitter.passthrough(ev);
                continue;
            }

            if (!vietnamese_enabled) {
                emitter.passthrough(ev);
                continue;
            }

            if (ctrl || alt || super_down) {
                engine.reset();
                emitter.passthrough(ev);
                continue;
            }

            if (is_navigation_or_reset(code)) {
                engine.reset();
                emitter.passthrough(ev);
                continue;
            }

            if (code == KEY_BACKSPACE) {
                int backs = 0;
                std::string replacement;
                if (engine.process_backspace(backs, replacement)) {
                    if (backs > 0) emitter.emit_backspaces(backs, cfg.micro_delay_us);
                    if (!replacement.empty()) emitter.emit_utf8_string(replacement, cfg.micro_delay_us);
                } else {
                    emitter.passthrough(ev);
                }
                continue;
            }

            char c = scancode_to_ascii(code, shift, capslock_on);
            if (c == 0) {
                engine.reset();
                emitter.passthrough(ev);
                continue;
            }

            int backs = 0;
            std::string replacement;
            if (engine.process_key(c, backs, replacement)) {
                if (backs > 0) emitter.emit_backspaces(backs, cfg.micro_delay_us);
                if (!replacement.empty()) emitter.emit_utf8_string(replacement, cfg.micro_delay_us);
            } else {
                emitter.passthrough(ev);
            }
        }
    }

    Logger::info("Exiting SenKey");
    ipc.stop();
    mouse_watcher.stop();
    hotplug.stop();
    grabber.close_all();
    emitter.close_device();

    return 0;
}
