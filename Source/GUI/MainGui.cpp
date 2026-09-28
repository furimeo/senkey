// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <gtk/gtk.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <chrono>
#include "MainWindow.hpp"
#include "Tray/TrayManager.hpp"
#include "Ipc.hpp"

static senkey::MainWindow* g_main_window = nullptr;
static senkey::TrayManager* g_tray_manager = nullptr;
static bool g_start_minimized = false;

static void on_app_activate(GtkApplication* app, gpointer) {
    if (!g_main_window) {
        g_application_hold(G_APPLICATION(app)); // Giữ ứng dụng luôn chạy ngầm cùng System Tray
        g_main_window = new senkey::MainWindow();
        g_tray_manager = new senkey::TrayManager(g_main_window);
        gtk_application_add_window(app, GTK_WINDOW(g_main_window->get_widget()));

        if (!g_start_minimized) {
            g_main_window->present();
        }
    } else {
        // Đã có ứng dụng đang chạy -> kích hoạt cửa sổ lên trên cùng
        g_main_window->present();
    }
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tray" || arg == "--minimized" || arg == "-t") {
            g_start_minimized = true;
        }
    }

    // Đảm bảo dịch vụ nền SenKey luôn hoạt động
    std::string status_resp;
    if (!senkey::IpcServer::send_command("STATUS", status_resp)) {
        if (system("systemctl --user start senkey.service 2>/dev/null") != 0 ||
            !senkey::IpcServer::send_command("STATUS", status_resp)) {
            pid_t pid = fork();
            if (pid == 0) {
                setsid();
                execlp("senkey", "senkey", "--service", nullptr);
                _exit(1);
            }
        }
    }

    GtkApplication* app = gtk_application_new("com.furimeo.senkey", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), nullptr);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    delete g_tray_manager;
    delete g_main_window;
    g_object_unref(app);

    return status;
}
