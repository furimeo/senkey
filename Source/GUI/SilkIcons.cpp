// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "SilkIcons.hpp"
#include <gio/gio.h>

namespace senkey {

GdkPixbuf* silk_icon(const char* name) {
    std::string path = std::string("/senkey/icons/") + name;
    GError* err = nullptr;
    GBytes* bytes = g_resources_lookup_data(path.c_str(), G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
    if (!bytes) {
        if (err) g_error_free(err);
        return nullptr;
    }
    gsize size = 0;
    const void* data = g_bytes_get_data(bytes, &size);
    GdkPixbuf* pb = nullptr;
    GInputStream* stream = g_memory_input_stream_new_from_data(data, static_cast<gssize>(size), nullptr);
    if (stream) {
        pb = gdk_pixbuf_new_from_stream(G_INPUT_STREAM(stream), nullptr, nullptr);
        g_object_unref(stream);
    }
    g_bytes_unref(bytes);
    return pb;
}

GtkWidget* silk_image(const char* name) {
    GdkPixbuf* pb = silk_icon(name);
    if (!pb) return gtk_image_new();
    GtkWidget* img = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    return img;
}

GtkWidget* icon_label_box(const char* icon_name, const char* label_text, int spacing) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
    gtk_box_pack_start(GTK_BOX(box), silk_image(icon_name), FALSE, FALSE, 0);
    GtkWidget* lbl = gtk_label_new(label_text);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    return box;
}

} // namespace senkey
