// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <functional>
#include "Config.hpp"

namespace senkey {

class TrayMenu {
private:
    GtkWidget* menu{nullptr};
    ConfigManager cfg_mgr;

    GtkWidget* item_mode_vi{nullptr};
    GtkWidget* item_mode_en{nullptr};
    GtkWidget* item_im_telex{nullptr};
    GtkWidget* item_im_vni{nullptr};
    GtkWidget* item_im_simple{nullptr};
    GtkWidget* item_cs_unicode{nullptr};
    GtkWidget* item_cs_tcvn3{nullptr};
    GtkWidget* item_cs_vni{nullptr};
    GtkWidget* item_chk_spell{nullptr};
    GtkWidget* item_chk_macro{nullptr};

    std::function<void(bool)> on_mode_toggled;
    std::function<void()> on_open_panel;
    std::function<void()> on_open_about;
    std::function<void()> on_quit_requested;

    void build_menu_items();

public:
    TrayMenu();
    ~TrayMenu() = default;

    GtkWidget* get_widget() const { return menu; }
    GtkWidget* get_mode_vi_item() const { return item_mode_vi; }

    void set_on_mode_toggled(std::function<void(bool)> cb) { on_mode_toggled = cb; }
    void set_on_open_panel(std::function<void()> cb)       { on_open_panel = cb; }
    void set_on_open_about(std::function<void()> cb)       { on_open_about = cb; }
    void set_on_quit_requested(std::function<void()> cb)   { on_quit_requested = cb; }

    void update_state(bool is_vietnamese);
    void popup(guint button, guint32 activate_time, GtkStatusIcon* status_icon = nullptr);
};

} // namespace senkey
