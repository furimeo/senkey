// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "ButtonBar.hpp"
#include "SilkIcons.hpp"

namespace senkey {

static GtkWidget* icon_btn(const char* icon, const char* label) {
    GtkWidget* btn = gtk_button_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(box), silk_image(icon), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new(label), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn), box);
    return btn;
}

ButtonBar::ButtonBar() {
    container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    expand_btn = icon_btn("application_side_expand.png", "Mở rộng");
    g_signal_connect(expand_btn, "clicked", G_CALLBACK(on_expand_clicked), this);
    gtk_box_pack_start(GTK_BOX(container), expand_btn, FALSE, FALSE, 0);

    about_btn = icon_btn("information.png", "Về SenKey");
    g_signal_connect(about_btn, "clicked", G_CALLBACK(on_about_clicked), this);
    gtk_box_pack_start(GTK_BOX(container), about_btn, FALSE, FALSE, 0);

    quit_btn = icon_btn("cross.png", "Kết thúc");
    g_signal_connect(quit_btn, "clicked", G_CALLBACK(on_quit_clicked), this);
    gtk_box_pack_end(GTK_BOX(container), quit_btn, FALSE, FALSE, 0);

    close_btn = icon_btn("tick.png", "Đóng");
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), this);
    gtk_box_pack_end(GTK_BOX(container), close_btn, FALSE, FALSE, 0);
}

void ButtonBar::on_expand_clicked(GtkButton*, gpointer data) {
    static_cast<ButtonBar*>(data)->expand_cb();
}
void ButtonBar::on_close_clicked(GtkButton*, gpointer data) {
    static_cast<ButtonBar*>(data)->close_cb();
}
void ButtonBar::on_quit_clicked(GtkButton*, gpointer data) {
    static_cast<ButtonBar*>(data)->quit_cb();
}
void ButtonBar::on_about_clicked(GtkButton*, gpointer data) {
    if (static_cast<ButtonBar*>(data)->about_cb)
        static_cast<ButtonBar*>(data)->about_cb();
}

void ButtonBar::update_expand_label(bool revealed) {
    const char* icon  = revealed ? "application_side_contract.png" : "application_side_expand.png";
    const char* label = revealed ? "Thu nhỏ" : "Mở rộng";

    GtkWidget* box = gtk_bin_get_child(GTK_BIN(expand_btn));
    GList* ch = gtk_container_get_children(GTK_CONTAINER(box));
    for (GList* l = ch; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(ch);

    gtk_box_pack_start(GTK_BOX(box), silk_image(icon),       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new(label),   FALSE, FALSE, 0);
    gtk_widget_show_all(expand_btn);
}

} // namespace senkey
