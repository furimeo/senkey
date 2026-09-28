// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "TrayBackend.hpp"
#include "SilkIcons.hpp"
#include <dlfcn.h>

namespace senkey {

// Định nghĩa con trỏ hàm cho AppIndicator API
typedef void* (*fn_app_indicator_new)(const char*, const char*, int);
typedef void  (*fn_app_indicator_set_icon_theme_path)(void*, const char*);
typedef void  (*fn_app_indicator_set_title)(void*, const char*);
typedef void  (*fn_app_indicator_set_status)(void*, int);
typedef void  (*fn_app_indicator_set_menu)(void*, GtkMenu*);
typedef void  (*fn_app_indicator_set_secondary_activate_target)(void*, GtkWidget*);
typedef void  (*fn_app_indicator_set_icon_full)(void*, const char*, const char*);

static fn_app_indicator_new p_new = nullptr;
static fn_app_indicator_set_icon_theme_path p_set_icon_theme_path = nullptr;
static fn_app_indicator_set_title p_set_title = nullptr;
static fn_app_indicator_set_status p_set_status = nullptr;
static fn_app_indicator_set_menu p_set_menu = nullptr;
static fn_app_indicator_set_secondary_activate_target p_set_sec = nullptr;
static fn_app_indicator_set_icon_full p_set_icon_full = nullptr;

TrayBackend::TrayBackend() = default;

TrayBackend::~TrayBackend() {
    if (indicator) {
        g_object_unref(indicator);
        indicator = nullptr;
    }
    if (lib_handle) {
        dlclose(lib_handle);
        lib_handle = nullptr;
    }
    if (status_icon) {
        g_object_unref(status_icon);
        status_icon = nullptr;
    }
}

bool TrayBackend::init_app_indicator(GtkWidget* menu, GtkWidget* secondary_item) {
    const char* libs[] = {
        "libayatana-appindicator3.so.1",
        "libayatana-appindicator3.so",
        "libappindicator3.so.1",
        "libappindicator3.so"
    };

    for (const char* lib_name : libs) {
        lib_handle = dlopen(lib_name, RTLD_LAZY);
        if (lib_handle) break;
    }

    if (!lib_handle) {
        return false;
    }

    p_new = (fn_app_indicator_new)dlsym(lib_handle, "app_indicator_new");
    p_set_icon_theme_path = (fn_app_indicator_set_icon_theme_path)dlsym(lib_handle, "app_indicator_set_icon_theme_path");
    p_set_title = (fn_app_indicator_set_title)dlsym(lib_handle, "app_indicator_set_title");
    p_set_status = (fn_app_indicator_set_status)dlsym(lib_handle, "app_indicator_set_status");
    p_set_menu = (fn_app_indicator_set_menu)dlsym(lib_handle, "app_indicator_set_menu");
    p_set_sec = (fn_app_indicator_set_secondary_activate_target)dlsym(lib_handle, "app_indicator_set_secondary_activate_target");
    p_set_icon_full = (fn_app_indicator_set_icon_full)dlsym(lib_handle, "app_indicator_set_icon_full");

    if (!p_new || !p_set_status || !p_set_menu) {
        dlclose(lib_handle);
        lib_handle = nullptr;
        return false;
    }

    // 0 = APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    // 1 = APP_INDICATOR_STATUS_ACTIVE
    indicator = p_new("senkey", "senkey-v", 0);
    if (!indicator) {
        dlclose(lib_handle);
        lib_handle = nullptr;
        return false;
    }

    if (p_set_icon_theme_path) p_set_icon_theme_path(indicator, icon_dir.c_str());
    if (p_set_title) p_set_title(indicator, "SenKey");
    p_set_status(indicator, 1);
    p_set_menu(indicator, GTK_MENU(menu));
    if (secondary_item && p_set_sec) {
        p_set_sec(indicator, secondary_item);
    }
    return true;
}

bool TrayBackend::init(const std::string& dir, GtkWidget* menu, GtkWidget* secondary_item) {
    icon_dir = dir;

    // Thử khởi tạo AppIndicator động nếu hệ thống có thư viện
    if (init_app_indicator(menu, secondary_item)) {
        return true;
    }

    // Dự phòng qua GtkStatusIcon (chuẩn GTK3)
    status_icon = gtk_status_icon_new();
    GdkPixbuf* pb = silk_icon("senkey-v.png");
    if (pb) {
        gtk_status_icon_set_from_pixbuf(status_icon, pb);
        g_object_unref(pb);
    }
    gtk_status_icon_set_title(status_icon, "SenKey");
    gtk_status_icon_set_tooltip_text(status_icon, "SenKey - Bộ gõ tiếng Việt");
    gtk_status_icon_set_visible(status_icon, TRUE);

    g_signal_connect_swapped(status_icon, "activate", G_CALLBACK(+[](gpointer data) {
        auto* self = static_cast<TrayBackend*>(data);
        if (self->on_activate_cb) self->on_activate_cb();
    }), this);

    g_signal_connect_swapped(status_icon, "popup-menu", G_CALLBACK(+[](gpointer data, guint button, guint32 time) {
        auto* self = static_cast<TrayBackend*>(data);
        if (self->on_popup_menu_cb) self->on_popup_menu_cb(button, time);
    }), this);

    return true;
}

void TrayBackend::update_icon(bool is_vietnamese) {
    const char* icon_name = is_vietnamese ? "senkey-v" : "senkey-e";
    const char* desc = is_vietnamese ? "SenKey [Tiếng Việt]" : "SenKey [Tiếng Anh]";

    if (indicator && p_set_icon_full) {
        p_set_icon_full(indicator, icon_name, desc);
    }

    if (status_icon) {
        GdkPixbuf* pb = silk_icon(is_vietnamese ? "senkey-v.png" : "senkey-e.png");
        if (pb) {
            gtk_status_icon_set_from_pixbuf(status_icon, pb);
            g_object_unref(pb);
        }
        gtk_status_icon_set_tooltip_text(status_icon, desc);
    }
}

} // namespace senkey
