// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <string>
#include <functional>

namespace senkey {

class TrayBackend {
private:
    void* lib_handle{nullptr};
    void* indicator{nullptr};
    GtkStatusIcon* status_icon{nullptr};
    std::string icon_dir;

    std::function<void()> on_activate_cb;
    std::function<void(guint, guint32)> on_popup_menu_cb;

    bool init_app_indicator(GtkWidget* menu, GtkWidget* secondary_item);

public:
    TrayBackend();
    ~TrayBackend();

    bool init(const std::string& dir, GtkWidget* menu, GtkWidget* secondary_item);
    void update_icon(bool is_vietnamese);

    void set_on_activate(std::function<void()> cb) { on_activate_cb = cb; }
    void set_on_popup_menu(std::function<void(guint, guint32)> cb) { on_popup_menu_cb = cb; }

    GtkStatusIcon* get_status_icon() const { return status_icon; }
};

} // namespace senkey
