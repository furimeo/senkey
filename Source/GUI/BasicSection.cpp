// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "BasicSection.hpp"
#include "SilkIcons.hpp"

namespace senkey {

BasicSection::BasicSection() {
    container = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(container), 8);
    gtk_grid_set_column_spacing(GTK_GRID(container), 12);

    gtk_grid_attach(GTK_GRID(container), icon_label_box("world.png",    "Bảng mã:"),    0, 0, 1, 1);
    charset_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset_combo), "Unicode (UTF-8)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset_combo), "TCVN3 (ABC)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset_combo), "VNI Windows");
    gtk_widget_set_hexpand(charset_combo, TRUE);
    gtk_grid_attach(GTK_GRID(container), charset_combo, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(container), icon_label_box("keyboard.png", "Kiểu gõ:"),    0, 1, 1, 1);
    im_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(im_combo), "Telex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(im_combo), "VNI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(im_combo), "Simple Telex");
    gtk_widget_set_hexpand(im_combo, TRUE);
    gtk_grid_attach(GTK_GRID(container), im_combo, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(container), icon_label_box("wrench.png",   "Phím chuyển:"), 0, 2, 1, 1);
    GtkWidget* hk_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    opt_ctrl_shift = gtk_radio_button_new_with_label(nullptr, "Ctrl + Shift");
    opt_alt_z      = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(opt_ctrl_shift), "Alt + Z");
    gtk_box_pack_start(GTK_BOX(hk_box), opt_ctrl_shift, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hk_box), opt_alt_z, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(container), hk_box, 1, 2, 1, 1);
}

void BasicSection::load(const SenKeyConfig& cfg) {
    gtk_combo_box_set_active(GTK_COMBO_BOX(charset_combo), static_cast<int>(cfg.charset));
    gtk_combo_box_set_active(GTK_COMBO_BOX(im_combo), static_cast<int>(cfg.input_method));
    if (cfg.hotkey == HotkeyToggle::ALT_Z)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opt_alt_z), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opt_ctrl_shift), TRUE);
}

void BasicSection::apply(SenKeyConfig& cfg) {
    int cs = gtk_combo_box_get_active(GTK_COMBO_BOX(charset_combo));
    cfg.charset = (cs == 1) ? Charset::TCVN3
                : (cs == 2) ? Charset::VNI_WINDOWS
                :              Charset::UNICODE_UTF8;

    int im = gtk_combo_box_get_active(GTK_COMBO_BOX(im_combo));
    cfg.input_method = (im == 1) ? InputMethod::VNI
                     : (im == 2) ? InputMethod::SIMPLE_TELEX
                     :              InputMethod::TELEX;

    cfg.hotkey = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(opt_alt_z))
               ? HotkeyToggle::ALT_Z
               : HotkeyToggle::CTRL_SHIFT;
}

} // namespace senkey
