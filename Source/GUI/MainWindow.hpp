// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include "BasicSection.hpp"
#include "AdvancedSection.hpp"
#include "ButtonBar.hpp"
#include "AboutDialog.hpp"
#include "Config.hpp"

namespace senkey {

class MainWindow {
private:
    GtkWidget* window;
    BasicSection basic_section;
    AdvancedSection advanced_section;
    ButtonBar button_bar;
    AboutDialog about_dialog;
    ConfigManager cfg_mgr;

    void on_expand();
    void on_save_close();
    void on_quit_daemon();
    void on_about();

    void resize_to_fit();

public:
    MainWindow();
    ~MainWindow() = default;

    void show_all();
    GtkWidget* get_widget() const { return window; }
};

} // namespace senkey
