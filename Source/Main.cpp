// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <set>
#include <memory>
#include <thread>
#include <unordered_set>
#include <unistd.h>
#include <fcntl.h>

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
#include "Pipeline/EventQueue.hpp"
#include "Pipeline/IpcCommandQueue.hpp"
#include "Input/XkbState.hpp"

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

static bool launch_gui_detached() {
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        setsid();
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("senkey-gui", "senkey-gui", nullptr);
        execlp("./senkey-gui", "./senkey-gui", nullptr);
        _exit(1);
    }
    return true;
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
            if (launch_gui_detached()) {
                std::cout << "SenKey GUI control panel launched.\n";
                return 0;
            }
            std::cerr << "Failed to fork GUI process.\n";
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
            // Daemon nền đang chạy -> Mở giao diện tách biệt ở background và thoát ngay lập tức để giải phóng terminal
            std::cout << "SenKey is running in background (Mode: [" << status_resp << "]).\n";
            std::cout << "Launching SenKey control panel...\n";
            launch_gui_detached();
            return 0;
        }

        // Tự động chuyển vào nền (daemonize) để giải phóng terminal
        std::cout << "Starting SenKey background service...\n";
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

    IpcCommandQueue ipc_cmd_queue;
    ipc_cmd_queue.set_wake_callback([&grabber]() {
        grabber.wake();
    });

    IpcServer ipc;
    ipc.start([&](const std::string& cmd) -> std::string {
        if (cmd == "STATUS") {
            // Read-only, lock-free, zero thread contention
            return vietnamese_enabled.load() ? "V" : "E";
        } else if (cmd == "QUIT") {
            g_running = false;
            grabber.wake();
            return "OK";
        } else if (cmd == "TOGGLE" || cmd == "RELOAD") {
            // Mutating commands routed to main thread as sole owner of EngineWrapper
            auto prom = std::make_shared<std::promise<std::string>>();
            auto fut = prom->get_future();
            ipc_cmd_queue.push({cmd, prom});
            if (fut.wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
                return fut.get();
            }
            return "TIMEOUT";
        }
        return "ERR";
    });

    ModifierState mod;
    mod.capslock = grabber.is_capslock_on();

    std::set<std::pair<int, int>> consumed_keys;
    std::vector<KeyEvent> events;
    events.reserve(32);

    XkbState xkb_state;
    emitter.set_xkb_state(&xkb_state);
    EventQueue event_queue;

    std::thread emitter_worker([&event_queue, &emitter]() {
        OutputAction action;
        while (event_queue.pop(action)) {
            if (action.type == ActionType::PASSTHROUGH) {
                emitter.emit_passthrough(action.raw_event);
            } else if (action.type == ActionType::REPLACEMENT) {
                emitter.emit_replacement(action.backs, action.replacement, action.mod);
            }
        }
    });

    Logger::info("SenKey ready (" + std::string(vietnamese_enabled.load() ? "V" : "E") + ")");

    while (g_running) {
        // Safely drain and process pending IPC commands on the single owner thread
        auto pending_cmds = ipc_cmd_queue.drain();
        for (auto& req : pending_cmds) {
            if (req.command == "TOGGLE") {
                vietnamese_enabled = !vietnamese_enabled.load();
                engine.reset();
                consumed_keys.clear();
                Logger::info(vietnamese_enabled.load() ? "Mode: [V]" : "Mode: [E]");
                req.promise->set_value(vietnamese_enabled.load() ? "V" : "E");
            } else if (req.command == "RELOAD") {
                cfg_mgr.load();
                engine.apply_config(cfg_mgr.get());
                engine.load_macros();
                Logger::info("Configuration and macros reloaded.");
                req.promise->set_value("OK");
            } else {
                req.promise->set_value("ERR");
            }
        }

        if (device_changed.exchange(false)) {
            Logger::info("Hardware hotplug event. Updating grabbed keyboards...");
            grabber.init_and_grab_all();
            mod.capslock = grabber.is_capslock_on();
            consumed_keys.clear();
        }

        if (mouse_clicked.exchange(false)) {
            engine.reset();
            consumed_keys.clear();
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

        for (const auto& key_ev : events) {
            const auto& ev = key_ev.ev;
            int dev_id = key_ev.device_id;

            if (ev.type != EV_KEY) {
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
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
                    Logger::info(vietnamese_enabled ? "Mode: [V]" : "Mode: [E]");
                    continue;
                }
            }

            // Key release (val == 0)
            if (val == 0) {
                xkb_state.process_key(code, 0, mod);
                if (consumed_keys.erase({dev_id, code}) > 0) {
                    // Physical key was absorbed by the Vietnamese engine on press.
                    // uinput never received a key-down for it, so do not pass key-up.
                    continue;
                }
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            // Autorepeat handling (val == 2)
            if (val == 2) {
                if (consumed_keys.count({dev_id, code}) > 0) {
                    // This key was consumed by the Vietnamese engine. Drop autorepeat.
                    continue;
                }
                if (code == KEY_BACKSPACE && vietnamese_enabled && !ctrl && !alt && !mod.super) {
                    int backs = 0;
                    std::string replacement;
                    if (engine.process_backspace(backs, replacement)) {
                        event_queue.push({ActionType::REPLACEMENT, {}, backs, replacement, mod});
                    } else {
                        event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                    }
                    continue;
                }
                if (is_navigation_or_reset(code)) {
                    engine.reset();
                    consumed_keys.clear();
                    event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                    continue;
                }
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            // From here on, val == 1 (Key Down)
            if (!vietnamese_enabled) {
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            if (ctrl || alt || mod.super) {
                engine.reset();
                consumed_keys.clear();
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            if (is_navigation_or_reset(code)) {
                engine.reset();
                consumed_keys.clear();
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            if (is_modifier(code)) {
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            if (code == KEY_BACKSPACE) {
                int backs = 0;
                std::string replacement;
                if (engine.process_backspace(backs, replacement)) {
                    event_queue.push({ActionType::REPLACEMENT, {}, backs, replacement, mod});
                    consumed_keys.insert({dev_id, code});
                } else {
                    event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                }
                continue;
            }

            char c = xkb_state.process_key(code, 1, mod);
            if (c == 0) {
                engine.reset();
                consumed_keys.clear();
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
                continue;
            }

            int backs = 0;
            std::string replacement;
            if (engine.process_key(c, backs, replacement)) {
                event_queue.push({ActionType::REPLACEMENT, {}, backs, replacement, mod});
                consumed_keys.insert({dev_id, code});
            } else {
                event_queue.push({ActionType::PASSTHROUGH, ev, 0, "", {}});
            }
        }
    }

    Logger::info("Exiting SenKey");
    event_queue.stop();
    if (emitter_worker.joinable()) {
        emitter_worker.join();
    }
    ipc.stop();
    mouse_watcher.stop();
    hotplug.stop();
    grabber.close_all();
    emitter.close_device();

    return 0;
}
