// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include "TrayIcons.hpp"
#include "TrayMenu.hpp"
#include "TrayBackend.hpp"

namespace senkey {

class MainWindow;

class TrayManager {
private:
    MainWindow* main_window{nullptr};
    TrayMenu tray_menu;
    TrayBackend tray_backend;
    bool is_vietnamese_mode{true};
    guint poll_timer_id{0};

    void poll_status();
    static gboolean on_poll_timer(gpointer user_data);

public:
    explicit TrayManager(MainWindow* window);
    ~TrayManager();

    void toggle_mode();
    void set_mode(bool vi_mode);
    bool get_mode() const { return is_vietnamese_mode; }
    void set_visible(bool visible) { tray_backend.set_visible(visible); }

    void show_panel();
    void quit_application();
};

} // namespace senkey
