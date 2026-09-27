// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "MainWindow.hpp"
#include "Ipc.hpp"
#include "SilkIcons.hpp"
#include <string>

namespace senkey {

MainWindow::MainWindow() {
    cfg_mgr.load();
    const SenKeyConfig& cfg = cfg_mgr.get();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "SenKey - Bảng điều khiển");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GdkPixbuf* kb = silk_icon("keyboard.png");
    if (kb) { gtk_window_set_icon(GTK_WINDOW(window), kb); g_object_unref(kb); }

    GtkWidget* root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(window), root_box);

    basic_section.load(cfg);
    gtk_box_pack_start(GTK_BOX(root_box), basic_section.get_widget(), FALSE, FALSE, 0);

    advanced_section.load(cfg);
    gtk_box_pack_start(GTK_BOX(root_box), advanced_section.get_widget(), FALSE, FALSE, 0);

    about_dialog.set_parent(window);
    button_bar.set_on_expand([this]() { on_expand(); });
    button_bar.set_on_close([this]() { on_save_close(); });
    button_bar.set_on_quit([this]() { on_quit_daemon(); });
    button_bar.set_on_about([this]() { on_about(); });
    gtk_box_pack_end(GTK_BOX(root_box), button_bar.get_widget(), FALSE, FALSE, 0);
}

void MainWindow::resize_to_fit() {
    gtk_window_resize(GTK_WINDOW(window), 1, 1);
}

void MainWindow::on_expand() {
    advanced_section.toggle();
    button_bar.update_expand_label(advanced_section.is_revealed());

    if (!advanced_section.is_revealed()) {
        // Wait for 180ms revealer animation, then shrink window
        g_timeout_add(220, [](gpointer data) -> gboolean {
            static_cast<MainWindow*>(data)->resize_to_fit();
            return G_SOURCE_REMOVE;
        }, this);
    }
}

void MainWindow::on_save_close() {
    SenKeyConfig& cfg = cfg_mgr.get_mutable();
    basic_section.apply(cfg);
    advanced_section.apply(cfg);
    cfg_mgr.save();

    std::string resp;
    IpcServer::send_command("RELOAD", resp);
    gtk_widget_destroy(window);
}

void MainWindow::on_quit_daemon() {
    std::string resp;
    IpcServer::send_command("QUIT", resp);
    gtk_widget_destroy(window);
}

void MainWindow::on_about() {
    about_dialog.show();
}

void MainWindow::show_all() {
    gtk_widget_show_all(window);
}

} // namespace senkey
