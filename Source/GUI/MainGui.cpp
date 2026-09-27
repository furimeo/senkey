// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include <gtk/gtk.h>
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    senkey::MainWindow main_window;
    main_window.show_all();
    gtk_main();
    return 0;
}
