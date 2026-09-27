// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <gtk/gtk.h>
#include <string>
#include "MainWindow.hpp"
#include "Tray/TrayManager.hpp"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    bool start_minimized = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tray" || arg == "--minimized" || arg == "-t") {
            start_minimized = true;
        }
    }

    senkey::MainWindow main_window;
    senkey::TrayManager tray_manager(&main_window);

    if (!start_minimized) {
        main_window.present();
    }

    gtk_main();
    return 0;
}
