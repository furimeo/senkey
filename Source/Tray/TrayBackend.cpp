// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "TrayBackend.hpp"
#include "SilkIcons.hpp"

namespace senkey {

TrayBackend::TrayBackend() = default;

TrayBackend::~TrayBackend() {
#if defined(HAVE_AYATANA_APPINDICATOR)
    if (indicator) {
        g_object_unref(indicator);
        indicator = nullptr;
    }
#endif
    if (status_icon) {
        g_object_unref(status_icon);
        status_icon = nullptr;
    }
}

bool TrayBackend::init(const std::string& dir, GtkWidget* menu, GtkWidget* secondary_item) {
    icon_dir = dir;

#if defined(HAVE_AYATANA_APPINDICATOR)
    indicator = app_indicator_new(
        "senkey",
        "senkey-v",
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    );

    if (indicator) {
        app_indicator_set_icon_theme_path(indicator, icon_dir.c_str());
        app_indicator_set_title(indicator, "SenKey");
        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_menu(indicator, GTK_MENU(menu));
        if (secondary_item) {
            app_indicator_set_secondary_activate_target(indicator, secondary_item);
        }
        return true;
    }
#endif

    // Dự phòng qua GtkStatusIcon
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

#if defined(HAVE_AYATANA_APPINDICATOR)
    if (indicator) {
        app_indicator_set_icon_full(indicator, icon_name, desc);
    }
#endif

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
