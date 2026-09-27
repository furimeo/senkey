// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "TrayMenu.hpp"
#include "Ipc.hpp"

namespace senkey {

TrayMenu::TrayMenu() {
    menu = gtk_menu_new();
    build_menu_items();
}

void TrayMenu::build_menu_items() {
    // Tiêu đề
    GtkWidget* title = gtk_menu_item_new_with_label("SenKey 1.0.0");
    gtk_widget_set_sensitive(title, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), title);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Nhóm chế độ gõ
    GSList* mode_group = nullptr;
    item_mode_vi = gtk_radio_menu_item_new_with_label(mode_group, "Tiếng Việt [V]");
    mode_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_mode_vi));
    item_mode_en = gtk_radio_menu_item_new_with_label(mode_group, "Tiếng Anh [E]");

    g_signal_connect_swapped(item_mode_vi, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_mode_vi)) && self->on_mode_toggled) {
            self->on_mode_toggled(true);
        }
    }), this);

    g_signal_connect_swapped(item_mode_en, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_mode_en)) && self->on_mode_toggled) {
            self->on_mode_toggled(false);
        }
    }), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_mode_vi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_mode_en);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Mở bảng điều khiển
    GtkWidget* item_panel = gtk_menu_item_new_with_label("Bảng điều khiển...");
    g_signal_connect_swapped(item_panel, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (self->on_open_panel) self->on_open_panel();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_panel);

    // Menu con: Kiểu gõ
    GtkWidget* im_sub = gtk_menu_new();
    GtkWidget* item_im = gtk_menu_item_new_with_label("Kiểu gõ");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_im), im_sub);

    GSList* im_group = nullptr;
    item_im_telex = gtk_radio_menu_item_new_with_label(im_group, "Telex");
    im_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_im_telex));
    item_im_vni = gtk_radio_menu_item_new_with_label(im_group, "VNI");
    im_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_im_vni));
    item_im_simple = gtk_radio_menu_item_new_with_label(im_group, "Simple Telex");

    g_signal_connect_swapped(item_im_telex, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_im_telex))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().input_method = InputMethod::TELEX;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    g_signal_connect_swapped(item_im_vni, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_im_vni))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().input_method = InputMethod::VNI;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    g_signal_connect_swapped(item_im_simple, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_im_simple))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().input_method = InputMethod::SIMPLE_TELEX;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(im_sub), item_im_telex);
    gtk_menu_shell_append(GTK_MENU_SHELL(im_sub), item_im_vni);
    gtk_menu_shell_append(GTK_MENU_SHELL(im_sub), item_im_simple);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_im);

    // Menu con: Bảng mã
    GtkWidget* cs_sub = gtk_menu_new();
    GtkWidget* item_cs = gtk_menu_item_new_with_label("Bảng mã");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_cs), cs_sub);

    GSList* cs_group = nullptr;
    item_cs_unicode = gtk_radio_menu_item_new_with_label(cs_group, "Unicode (UTF-8)");
    cs_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_cs_unicode));
    item_cs_tcvn3 = gtk_radio_menu_item_new_with_label(cs_group, "TCVN3 (ABC)");
    cs_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item_cs_tcvn3));
    item_cs_vni = gtk_radio_menu_item_new_with_label(cs_group, "VNI Windows");

    g_signal_connect_swapped(item_cs_unicode, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_cs_unicode))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().charset = Charset::UNICODE_UTF8;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    g_signal_connect_swapped(item_cs_tcvn3, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_cs_tcvn3))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().charset = Charset::TCVN3;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    g_signal_connect_swapped(item_cs_vni, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_cs_vni))) {
            self->cfg_mgr.load();
            self->cfg_mgr.get_mutable().charset = Charset::VNI_WINDOWS;
            self->cfg_mgr.save();
            std::string resp; IpcServer::send_command("RELOAD", resp);
        }
    }), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(cs_sub), item_cs_unicode);
    gtk_menu_shell_append(GTK_MENU_SHELL(cs_sub), item_cs_tcvn3);
    gtk_menu_shell_append(GTK_MENU_SHELL(cs_sub), item_cs_vni);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_cs);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Tùy chọn kiểm tra chính tả & gõ tắt
    item_chk_spell = gtk_check_menu_item_new_with_label("Kiểm tra chính tả");
    g_signal_connect_swapped(item_chk_spell, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        self->cfg_mgr.load();
        self->cfg_mgr.get_mutable().spell_check = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_chk_spell));
        self->cfg_mgr.save();
        std::string resp; IpcServer::send_command("RELOAD", resp);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_chk_spell);

    item_chk_macro = gtk_check_menu_item_new_with_label("Bật gõ tắt (Macro)");
    g_signal_connect_swapped(item_chk_macro, "toggled", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        self->cfg_mgr.load();
        self->cfg_mgr.get_mutable().macro_enabled = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self->item_chk_macro));
        self->cfg_mgr.save();
        std::string resp; IpcServer::send_command("RELOAD", resp);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_chk_macro);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Thông tin bản quyền
    GtkWidget* item_about = gtk_menu_item_new_with_label("Thông tin (Về SenKey)...");
    g_signal_connect_swapped(item_about, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (self->on_open_about) self->on_open_about();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_about);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Thoát
    GtkWidget* item_quit = gtk_menu_item_new_with_label("Thoát SenKey");
    g_signal_connect_swapped(item_quit, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayMenu*>(data);
        if (self->on_quit_requested) self->on_quit_requested();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_quit);

    gtk_widget_show_all(menu);
}

void TrayMenu::update_state(bool is_vietnamese) {
    cfg_mgr.load();
    const auto& c = cfg_mgr.get();

    if (item_mode_vi && item_mode_en) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(is_vietnamese ? item_mode_vi : item_mode_en), TRUE);
    }

    if (item_im_telex && item_im_vni && item_im_simple) {
        if (c.input_method == InputMethod::VNI) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_im_vni), TRUE);
        else if (c.input_method == InputMethod::SIMPLE_TELEX) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_im_simple), TRUE);
        else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_im_telex), TRUE);
    }

    if (item_cs_unicode && item_cs_tcvn3 && item_cs_vni) {
        if (c.charset == Charset::TCVN3) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_cs_tcvn3), TRUE);
        else if (c.charset == Charset::VNI_WINDOWS) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_cs_vni), TRUE);
        else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_cs_unicode), TRUE);
    }

    if (item_chk_spell) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_chk_spell), c.spell_check);
    if (item_chk_macro) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_chk_macro), c.macro_enabled);
}

void TrayMenu::popup(guint button, guint32 activate_time, GtkStatusIcon* status_icon) {
    if (status_icon) {
        gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr,
                        gtk_status_icon_position_menu, status_icon,
                        button, activate_time);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

} // namespace senkey
