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
#include <unordered_set>
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
            // Daemon nền đang chạy -> Mở/kích hoạt Bảng điều khiển giao diện (như UniKey)
            execlp("senkey-gui", "senkey-gui", nullptr);
            execlp("./senkey-gui", "./senkey-gui", nullptr);
            std::cout << "SenKey is running in background (Mode: [" << status_resp << "]).\n";
            return 0;
        }

        // Tự động chuyển vào nền (daemonize) để giải phóng terminal
        if (daemon(0, 0) != 0) {
            Logger::error("Failed to run SenKey in background");
            return 1;
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

    std::atomic<bool> device_changed{false};
    HotplugWatcher hotplug;
    hotplug.start([&device_changed]() {
        device_changed = true;
    });

    std::atomic<bool> mouse_clicked{false};
    MouseWatcher mouse_watcher;
    mouse_watcher.start([&mouse_clicked]() {
        mouse_clicked = true;
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

    ModifierState mod;
    mod.capslock = grabber.is_capslock_on();

    std::unordered_set<int> consumed_keys;
    std::vector<input_event> events;
    events.reserve(32);

    auto last_emission_time = std::chrono::steady_clock::now();
    bool emission_pending_barrier = false;

    Logger::info("SenKey ready (" + std::string(vietnamese_enabled ? "V" : "E") + ")");

    while (g_running) {
        if (device_changed.exchange(false)) {
            Logger::info("Hardware hotplug event. Updating grabbed keyboards...");
            grabber.init_and_grab_all();
            mod.capslock = grabber.is_capslock_on();
            consumed_keys.clear();
            emission_pending_barrier = false;
        }

        if (mouse_clicked.exchange(false)) {
            engine.reset();
            consumed_keys.clear();
            emission_pending_barrier = false;
        }

        if (grabber.grabbed_count() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            static int retry_ticks = 0;
            if (++retry_ticks >= 8) {
                retry_ticks = 0;
                grabber.init_and_grab_all();
                mod.capslock = grabber.is_capslock_on();
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

            if (code == KEY_LEFTSHIFT)  mod.lshift = (val > 0);
            if (code == KEY_RIGHTSHIFT) mod.rshift = (val > 0);
            if (code == KEY_LEFTCTRL)   mod.lctrl = (val > 0);
            if (code == KEY_RIGHTCTRL)  mod.rctrl = (val > 0);
            if (code == KEY_LEFTALT)    mod.lalt = (val > 0);
            if (code == KEY_RIGHTALT)   mod.ralt = (val > 0);
            if (code == KEY_LEFTMETA || code == KEY_RIGHTMETA) mod.super = (val > 0);
            if (code == KEY_CAPSLOCK && val == 1) mod.capslock = !mod.capslock;

            bool shift = mod.any_shift();
            bool ctrl = mod.any_ctrl();
            bool alt = mod.any_alt();

            if (val == 1) {
                bool trigger = false;
                if (cfg.hotkey == HotkeyToggle::CTRL_SHIFT) {
                    trigger = (ctrl && (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT)) ||
                              (shift && (code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL));
                } else if (cfg.hotkey == HotkeyToggle::ALT_Z) {
                    trigger = (alt && code == KEY_Z);
                } else if (cfg.hotkey == HotkeyToggle::SUPER_SPACE) {
                    trigger = (mod.super && code == KEY_SPACE);
                }

                if (trigger) {
                    vietnamese_enabled = !vietnamese_enabled;
                    engine.reset();
                    consumed_keys.clear();
                    emission_pending_barrier = false;
                    Logger::info(vietnamese_enabled ? "Mode: [V]" : "Mode: [E]");
                    continue;
                }
            }

            // Key release (val == 0)
            if (val == 0) {
                if (consumed_keys.erase(code) > 0) {
                    // Physical key was absorbed by the Vietnamese engine on press.
                    // uinput never received a key-down for it, so do not pass key-up.
                    continue;
                }
                emitter.passthrough(ev);
                continue;
            }

            // Autorepeat handling (val == 2)
            if (val == 2) {
                if (consumed_keys.count(code) > 0) {
                    // This key was consumed by the Vietnamese engine (e.g. 'r' diacritic).
                    // Drop autorepeat completely to prevent tone-reversal and infinite repeat spam.
                    continue;
                }
                if (code == KEY_BACKSPACE && vietnamese_enabled && !ctrl && !alt && !mod.super) {
                    int backs = 0;
                    std::string replacement;
                    if (engine.process_backspace(backs, replacement)) {
                        emitter.emit_replacement(backs, replacement, cfg.micro_delay_us, mod);
                        last_emission_time = std::chrono::steady_clock::now();
                        emission_pending_barrier = true;
                    } else {
                        emitter.passthrough(ev);
                    }
                    continue;
                }
                if (is_navigation_or_reset(code)) {
                    engine.reset();
                    consumed_keys.clear();
                    emission_pending_barrier = false;
                    emitter.passthrough(ev);
                    continue;
                }
                emitter.passthrough(ev);
                continue;
            }

            // From here on, val == 1 (Key Down)
            // Temporal Barrier: If a replacement was just committed, guarantee that target GUI applications
            // (Chrome, VSCode, GTK, Qt) have completely closed their preedit before the next keystroke is dispatched
            if (emission_pending_barrier) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_emission_time).count();
                if (elapsed_ms < 12) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(12 - elapsed_ms));
                }
                emission_pending_barrier = false;
            }

            if (!vietnamese_enabled) {
                emitter.passthrough(ev);
                continue;
            }

            if (ctrl || alt || mod.super) {
                engine.reset();
                consumed_keys.clear();
                emitter.passthrough(ev);
                continue;
            }

            if (is_navigation_or_reset(code)) {
                engine.reset();
                consumed_keys.clear();
                emitter.passthrough(ev);
                continue;
            }

            if (is_modifier(code)) {
                emitter.passthrough(ev);
                continue;
            }

            if (code == KEY_BACKSPACE) {
                int backs = 0;
                std::string replacement;
                if (engine.process_backspace(backs, replacement)) {
                    emitter.emit_replacement(backs, replacement, cfg.micro_delay_us, mod);
                    consumed_keys.insert(code);
                    last_emission_time = std::chrono::steady_clock::now();
                    emission_pending_barrier = true;
                } else {
                    emitter.passthrough(ev);
                }
                continue;
            }

            char c = scancode_to_ascii(code, shift, mod.capslock);
            if (c == 0) {
                engine.reset();
                consumed_keys.clear();
                emitter.passthrough(ev);
                continue;
            }

            int backs = 0;
            std::string replacement;
            if (engine.process_key(c, backs, replacement)) {
                emitter.emit_replacement(backs, replacement, cfg.micro_delay_us, mod);
                consumed_keys.insert(code);
                last_emission_time = std::chrono::steady_clock::now();
                emission_pending_barrier = true;
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
