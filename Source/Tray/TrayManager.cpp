// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "TrayManager.hpp"
#include "MainWindow.hpp"
#include "Ipc.hpp"

namespace senkey {

TrayManager::TrayManager(MainWindow* window)
    : main_window(window) {
    std::string icon_dir = TrayIcons::ensure_icon_directory();

    tray_backend.init(icon_dir, tray_menu.get_widget(), tray_menu.get_mode_vi_item());

    // Kết nối các tín hiệu từ menu
    tray_menu.set_on_mode_toggled([this](bool vi) {
        if (vi != is_vietnamese_mode) {
            toggle_mode();
        }
    });

    tray_menu.set_on_open_panel([this]() {
        show_panel();
    });

    tray_menu.set_on_open_about([this]() {
        if (main_window) {
            main_window->on_about_external();
        }
    });

    tray_menu.set_on_quit_requested([this]() {
        quit_application();
    });

    // Kết nối các tín hiệu từ backend khay hệ thống
    tray_backend.set_on_activate([this]() {
        toggle_mode();
    });

    tray_backend.set_on_popup_menu([this](guint button, guint32 time) {
        tray_menu.update_state(is_vietnamese_mode);
        tray_menu.popup(button, time, tray_backend.get_status_icon());
    });

    // Lấy trạng thái hiện tại từ daemon
    poll_status();

    // Chu kỳ định thời 300ms thăm dò trạng thái daemon
    poll_timer_id = g_timeout_add(300, on_poll_timer, this);
}

TrayManager::~TrayManager() {
    if (poll_timer_id > 0) {
        g_source_remove(poll_timer_id);
        poll_timer_id = 0;
    }
}

void TrayManager::toggle_mode() {
    std::string resp;
    if (IpcServer::send_command("TOGGLE", resp)) {
        set_mode(resp == "V");
    } else {
        set_mode(!is_vietnamese_mode);
    }
}

void TrayManager::set_mode(bool vi_mode) {
    if (is_vietnamese_mode != vi_mode) {
        is_vietnamese_mode = vi_mode;
        tray_backend.update_icon(is_vietnamese_mode);
        tray_menu.update_state(is_vietnamese_mode);
    }
}

void TrayManager::poll_status() {
    std::string resp;
    if (IpcServer::send_command("STATUS", resp)) {
        set_mode(resp == "V");
    }
}

gboolean TrayManager::on_poll_timer(gpointer user_data) {
    auto* self = static_cast<TrayManager*>(user_data);
    self->poll_status();
    return G_SOURCE_CONTINUE;
}

void TrayManager::show_panel() {
    if (main_window) {
        main_window->present();
    }
}

void TrayManager::quit_application() {
    std::string resp;
    IpcServer::send_command("QUIT", resp);
    gtk_main_quit();
}

} // namespace senkey
