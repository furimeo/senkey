// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <functional>

namespace senkey {

class ButtonBar {
private:
    GtkWidget* container;
    GtkWidget* expand_btn;
    GtkWidget* about_btn;
    GtkWidget* close_btn;
    GtkWidget* quit_btn;

    std::function<void()> expand_cb;
    std::function<void()> close_cb;
    std::function<void()> quit_cb;
    std::function<void()> about_cb;

    static void on_expand_clicked(GtkButton*, gpointer user_data);
    static void on_close_clicked(GtkButton*, gpointer user_data);
    static void on_quit_clicked(GtkButton*, gpointer user_data);
    static void on_about_clicked(GtkButton*, gpointer user_data);

public:
    ButtonBar();
    ~ButtonBar() = default;

    GtkWidget* get_widget() const { return container; }

    void set_on_expand(std::function<void()> cb) { expand_cb = cb; }
    void set_on_close(std::function<void()> cb)  { close_cb = cb; }
    void set_on_quit(std::function<void()> cb)   { quit_cb = cb; }
    void set_on_about(std::function<void()> cb)  { about_cb = cb; }

    void update_expand_label(bool is_revealed);
};

} // namespace senkey
