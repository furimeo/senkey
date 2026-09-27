// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "AboutDialog.hpp"
#include "SilkIcons.hpp"
#include "Version.hpp"
#include <string>

namespace senkey {

AboutDialog::AboutDialog(GtkWidget* parent_window)
    : parent(parent_window) {}

void AboutDialog::show() {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Về SenKey",
        parent ? GTK_WINDOW(parent) : nullptr,
        GTK_DIALOG_MODAL,
        "_Đóng", GTK_RESPONSE_CLOSE,
        nullptr
    );
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_spacing(GTK_BOX(content), 8);
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    // Biểu tượng ứng dụng và tiêu đề
    GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GdkPixbuf* kb_pb = silk_icon("keyboard.png");
    if (kb_pb) {
        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(kb_pb, 32, 32, GDK_INTERP_BILINEAR);
        gtk_box_pack_start(GTK_BOX(title_box), gtk_image_new_from_pixbuf(scaled), FALSE, FALSE, 0);
        g_object_unref(scaled);
        g_object_unref(kb_pb);
    }
    GtkWidget* name_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget* lbl_name = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(lbl_name), "<b><big>SenKey</big></b>");
    gtk_widget_set_halign(lbl_name, GTK_ALIGN_START);
    GtkWidget* lbl_desc = gtk_label_new("Bộ gõ tiếng Việt độc lập cho Linux");
    gtk_widget_set_halign(lbl_desc, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(name_box), lbl_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(name_box), lbl_desc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(title_box), name_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), title_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    auto add_row = [&](const char* icon, const char* key, const char* val) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_pack_start(GTK_BOX(row), silk_image(icon), FALSE, FALSE, 0);
        GtkWidget* lbl_k = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(lbl_k), (std::string("<b>") + key + "</b>").c_str());
        gtk_widget_set_halign(lbl_k, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(row), lbl_k, FALSE, FALSE, 0);
        GtkWidget* lbl_v = gtk_label_new(val);
        gtk_widget_set_halign(lbl_v, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(row), lbl_v, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(content), row, FALSE, FALSE, 0);
    };

    add_row("user.png",        "Tác giả:",              "Lê Hùng Quang Minh");
    add_row("user_suit.png",   "Tác giả lõi gõ:",       "Phạm Kim Long");
    add_row("world.png",       "Lõi xử lý:",            "UniKeyCore");
    add_row("cog.png",         "Phiên bản:",            senkey::VERSION);
    add_row("information.png", "Giấy phép SenKey:",     "GPL-2.0-or-later");
    add_row("information.png", "Giấy phép UniKeyCore:", "Tuân theo tác giả kèm NOTICE");

    gtk_box_pack_start(GTK_BOX(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 4);

    GtkWidget* note_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(note_row), silk_image("help.png"), FALSE, FALSE, 0);
    GtkWidget* lbl_note = gtk_label_new("SenKey phát hành dưới giấy phép GPL-2.0-or-later.\nPhần lõi UniKeyCore tuân theo bản quyền của Phạm Kim Long kèm NOTICE.");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_note), TRUE);
    gtk_widget_set_halign(lbl_note, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(note_row), lbl_note, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), note_row, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

} // namespace senkey
