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
#include <future>
#include <unordered_set>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_GTK3
#include <gtk/gtk.h>
#include "GUI/MainWindow.hpp"
#include "Tray/TrayManager.hpp"
#endif

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
#include "Pipeline/IpcCommandQueue.hpp"
#include "Input/XkbState.hpp"
#include "Output/Wayland/WlrVirtualKeyboardBackend.hpp"

using namespace senkey;

static std::atomic<bool> g_running{true};
static std::atomic<bool> g_vietnamese_enabled{true};
static KeyboardGrabber* g_grabber_ptr = nullptr;

#ifdef HAVE_GTK3
static MainWindow* g_main_window = nullptr;
static TrayManager* g_tray_manager = nullptr;
#endif

static void handle_signal(int sig) {
    (void)sig;
    g_running = false;
    if (g_grabber_ptr) {
        g_grabber_ptr->wake();
    }
#ifdef HAVE_GTK3
    g_idle_add(+[](gpointer) -> gboolean {
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }, nullptr);
#endif
}

static void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -g, --gui          Open graphical control panel\n"
              << "  -t, --toggle       Toggle active mode (V/E) of running service\n"
              << "  -s, --status       Print current mode (V or E) of running service\n"
              << "  -q, --quit         Terminate running service\n"
              << "  -r, --reload       Reload configuration and macros\n"
              << "      --no-tray      Disable system tray icon\n"
              << "      --tray         Start minimized in system tray (default)\n"
              << "  -c, --config PATH  Specify custom config file path\n"
              << "  -m, --macro PATH   Specify custom macro file path\n"
              << "      --verbose      Enable verbose debug logging\n"
              << "  -v, --version      Show version information\n"
              << "  -h, --help         Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool start_gui = false;
    bool no_tray = false;
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
        } else if (arg == "-t" || arg == "--toggle") {
            std::string resp;
            if (IpcServer::send_command("TOGGLE", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey is not running\n";
            return 1;
        } else if (arg == "-s" || arg == "--status") {
            std::string resp;
            if (IpcServer::send_command("STATUS", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey is not running\n";
            return 1;
        } else if (arg == "-q" || arg == "--quit") {
            std::string resp;
            if (IpcServer::send_command("QUIT", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey is not running\n";
            return 1;
        } else if (arg == "-r" || arg == "--reload") {
            std::string resp;
            if (IpcServer::send_command("RELOAD", resp)) {
                std::cout << resp << "\n";
                return 0;
            }
            std::cerr << "SenKey is not running\n";
            return 1;
        } else if (arg == "-g" || arg == "--gui") {
            std::string resp;
            if (IpcServer::send_command("GUI", resp)) {
                std::cout << "Activated SenKey control panel.\n";
                return 0;
            }
            start_gui = true;
        } else if (arg == "--no-tray") {
            no_tray = true;
        } else if (arg == "--tray" || arg == "--minimized" || arg == "--service") {
            start_gui = false;
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

    // Default: Check single-instance. If running, activate GUI.
    if (!start_gui) {
        std::string status_resp;
        if (IpcServer::send_command("GUI", status_resp)) {
            std::cout << "SenKey is already running (Mode: [" << status_resp << "]). Activated control panel.\n";
            return 0;
        }
    }

    if (verbose) {
        Logger::set_level(LogLevel::DEBUG);
    } else {
        Logger::set_level(LogLevel::INFO);
    }

#ifdef HAVE_GTK3
    bool has_gui = gtk_init_check(&argc, &argv);
    if (!has_gui) {
        Logger::warn("Display server unavailable for GTK. Running in headless mode.");
    }
#else
    bool has_gui = false;
#endif

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGHUP, handle_signal);

    ConfigManager cfg_mgr(custom_config);
    cfg_mgr.load();
    const SenKeyConfig& cfg = cfg_mgr.get();

    XkbState xkb_state;
    std::unique_ptr<OutputBackend> emitter;
#if defined(HAVE_WAYLAND)
    auto wlr_backend = std::make_unique<WlrVirtualKeyboardBackend>();
    if (wlr_backend->open_device("senkey-keyboard")) {
        Logger::info("Output backend: Wayland zwp_virtual_keyboard_v1 (Level 0 Native Unicode)");
        emitter = std::move(wlr_backend);
    }
#endif
    if (!emitter) {
        auto uinput_backend = std::make_unique<VirtualKeyboard>();
        uinput_backend->set_xkb_state(&xkb_state);
        if (!uinput_backend->open_device("senkey-keyboard")) {
            std::string err_str = std::strerror(errno);
            Logger::error("Failed to open /dev/uinput: " + err_str + 
                          ". Ensure your user is in the 'input' group (try running 'newgrp input' or re-login), "
                          "or verify the uinput kernel module is loaded.");
            return 1;
        }
        Logger::info("Output backend: Linux uinput virtual keyboard");
        emitter = std::move(uinput_backend);
    }

    KeyboardGrabber grabber;
    g_grabber_ptr = &grabber;
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

    IpcCommandQueue ipc_cmd_queue;
    ipc_cmd_queue.set_wake_callback([&grabber]() {
        grabber.wake();
    });

#ifdef HAVE_GTK3
    if (has_gui) {
        g_main_window = new MainWindow();
        if (cfg.show_tray && !no_tray) {
            g_tray_manager = new TrayManager(g_main_window);
        }
        if (start_gui) {
            g_main_window->present();
        }
    }
#endif

    IpcServer ipc;
    ipc.start([&](const std::string& cmd) -> std::string {
        if (cmd == "STATUS") {
            return g_vietnamese_enabled.load() ? "V" : "E";
        } else if (cmd == "QUIT") {
            g_running = false;
            grabber.wake();
#ifdef HAVE_GTK3
            if (has_gui) {
                g_idle_add(+[](gpointer) -> gboolean {
                    gtk_main_quit();
                    return G_SOURCE_REMOVE;
                }, nullptr);
            }
#endif
            return "OK";
        } else if (cmd == "GUI") {
#ifdef HAVE_GTK3
            if (has_gui && g_main_window) {
                g_idle_add(+[](gpointer data) -> gboolean {
                    if (g_main_window) {
                        g_main_window->present();
                    }
                    return G_SOURCE_REMOVE;
                }, nullptr);
            }
#endif
            return g_vietnamese_enabled.load() ? "V" : "E";
        } else if (cmd == "TOGGLE" || cmd == "RELOAD") {
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

    // Run-to-completion input worker: reads physical evdev events, transforms via UniKeyCore,
    // and directly synthesizes output via OutputBackend with zero queuing latency and zero packet dropping.
    std::thread input_worker([&]() {
        ModifierState mod;
        mod.capslock = grabber.is_capslock_on();

        std::set<std::pair<int, int>> consumed_keys;
        std::vector<KeyEvent> events;
        events.reserve(32);

        Logger::info("SenKey engine ready (" + std::string(g_vietnamese_enabled.load() ? "V" : "E") + ")");

        while (g_running) {
            auto pending_cmds = ipc_cmd_queue.drain();
            for (auto& req : pending_cmds) {
                if (req.command == "TOGGLE") {
                    g_vietnamese_enabled = !g_vietnamese_enabled.load();
                    engine.reset();
                    consumed_keys.clear();
                    bool is_vi = g_vietnamese_enabled.load();
                    Logger::info(is_vi ? "Mode: [V]" : "Mode: [E]");
#ifdef HAVE_GTK3
                    if (g_tray_manager) {
                        g_idle_add(+[](gpointer data) -> gboolean {
                            if (g_tray_manager) {
                                g_tray_manager->set_mode(data != nullptr);
                            }
                            return G_SOURCE_REMOVE;
                        }, is_vi ? reinterpret_cast<gpointer>(1) : nullptr);
                    }
#endif
                    req.promise->set_value(is_vi ? "V" : "E");
                } else if (req.command == "RELOAD") {
                    cfg_mgr.load();
                    engine.apply_config(cfg_mgr.get());
                    engine.load_macros();
#ifdef HAVE_GTK3
                    if (g_tray_manager) {
                        bool show = cfg_mgr.get().show_tray && !no_tray;
                        g_idle_add(+[](gpointer data) -> gboolean {
                            if (g_tray_manager) {
                                g_tray_manager->set_visible(data != nullptr);
                            }
                            return G_SOURCE_REMOVE;
                        }, show ? reinterpret_cast<gpointer>(1) : nullptr);
                    }
#endif
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
                    emitter->emit_passthrough(ev);
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
                        g_vietnamese_enabled = !g_vietnamese_enabled.load();
                        engine.reset();
                        consumed_keys.clear();
                        bool is_vi = g_vietnamese_enabled.load();
                        Logger::info(is_vi ? "Mode: [V]" : "Mode: [E]");
#ifdef HAVE_GTK3
                        if (g_tray_manager) {
                            g_idle_add(+[](gpointer data) -> gboolean {
                                if (g_tray_manager) {
                                    g_tray_manager->set_mode(data != nullptr);
                                }
                                return G_SOURCE_REMOVE;
                            }, is_vi ? reinterpret_cast<gpointer>(1) : nullptr);
                        }
#endif
                        continue;
                    }
                }

                if (val == 0) {
                    auto it = consumed_keys.find({dev_id, code});
                    if (it != consumed_keys.end()) {
                        consumed_keys.erase(it);
                        continue;
                    }
                    if (is_navigation_or_reset(code)) {
                        engine.reset();
                        consumed_keys.clear();
                        emitter->emit_passthrough(ev);
                        continue;
                    }
                    emitter->emit_passthrough(ev);
                    continue;
                }

                // Key Down (val == 1)
                if (!g_vietnamese_enabled.load()) {
                    emitter->emit_passthrough(ev);
                    continue;
                }

                if (ctrl || alt || mod.super) {
                    engine.reset();
                    consumed_keys.clear();
                    emitter->emit_passthrough(ev);
                    continue;
                }

                if (is_navigation_or_reset(code)) {
                    engine.reset();
                    consumed_keys.clear();
                    emitter->emit_passthrough(ev);
                    continue;
                }

                if (is_modifier(code)) {
                    emitter->emit_passthrough(ev);
                    continue;
                }

                if (code == KEY_BACKSPACE) {
                    int backs = 0;
                    std::string replacement;
                    if (engine.process_backspace(backs, replacement)) {
                        emitter->emit_replacement(backs, replacement, mod);
                        consumed_keys.insert({dev_id, code});
                    } else {
                        emitter->emit_passthrough(ev);
                    }
                    continue;
                }

                char c = xkb_state.process_key(code, 1, mod);
                if (c == 0) {
                    engine.reset();
                    consumed_keys.clear();
                    emitter->emit_passthrough(ev);
                    continue;
                }

                int backs = 0;
                std::string replacement;
                if (engine.process_key(c, backs, replacement)) {
                    emitter->emit_replacement(backs, replacement, mod);
                    consumed_keys.insert({dev_id, code});
                } else {
                    emitter->emit_passthrough(ev);
                }
            }
        }
    });

#ifdef HAVE_GTK3
    if (has_gui) {
        gtk_main();
    } else
#endif
    {
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    Logger::info("Exiting SenKey");
    g_running = false;
    grabber.wake();
    if (input_worker.joinable()) {
        input_worker.join();
    }
    ipc.stop();
    mouse_watcher.stop();
    hotplug.stop();
    grabber.close_all();
    if (emitter) {
        emitter->close_device();
    }

#ifdef HAVE_GTK3
    delete g_tray_manager;
    g_tray_manager = nullptr;
    delete g_main_window;
    g_main_window = nullptr;
#endif

    return 0;
}
