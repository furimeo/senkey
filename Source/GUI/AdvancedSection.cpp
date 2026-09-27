// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "AdvancedSection.hpp"
#include "SilkIcons.hpp"

namespace senkey {

static GtkWidget* check_row(GtkWidget** out_chk, const char* icon, const char* label) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(row), silk_image(icon), FALSE, FALSE, 0);
    *out_chk = gtk_check_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(row), *out_chk, FALSE, FALSE, 0);
    return row;
}

AdvancedSection::AdvancedSection() {
    revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 180);

    GtkWidget* frame = gtk_frame_new(nullptr);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(revealer), frame);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 8);
    gtk_container_add(GTK_CONTAINER(frame), box);

    gtk_box_pack_start(GTK_BOX(box), check_row(&chk_modern, "tick.png",         "Đặt dấu chuẩn (oà, uỳ)"),   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), check_row(&chk_free,   "text_replace.png", "Cho phép gõ tự do"),          FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), check_row(&chk_spell,  "accept.png",        "Bật kiểm tra chính tả"),     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), check_row(&chk_macro,  "script.png",        "Bật gõ tắt (Macro)"),        FALSE, FALSE, 0);

    GtkWidget* pacing = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(pacing), silk_image("cog.png"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pacing), gtk_label_new("Micro-pacing (µs):"), FALSE, FALSE, 0);
    spin_delay = gtk_spin_button_new_with_range(500, 5000, 100);
    gtk_box_pack_start(GTK_BOX(pacing), spin_delay, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), pacing, FALSE, FALSE, 0);
}

void AdvancedSection::load(const SenKeyConfig& cfg) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_modern), cfg.modern_spelling);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_free),   cfg.free_marking);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_spell),  cfg.spell_check);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_macro),  cfg.macro_enabled);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_delay), cfg.micro_delay_us);
}

void AdvancedSection::apply(SenKeyConfig& cfg) {
    cfg.modern_spelling = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_modern));
    cfg.free_marking    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_free));
    cfg.spell_check     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_spell));
    cfg.macro_enabled   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_macro));
    cfg.micro_delay_us  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_delay));
}

void AdvancedSection::toggle() {
    gboolean cur = gtk_revealer_get_reveal_child(GTK_REVEALER(revealer));
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), !cur);
}

bool AdvancedSection::is_revealed() const {
    return gtk_revealer_get_reveal_child(GTK_REVEALER(revealer));
}

} // namespace senkey
