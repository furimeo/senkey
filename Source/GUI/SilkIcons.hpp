// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <string>

namespace senkey {

// Load a silk icon from the embedded GResource bundle.
// Resource path: /senkey/icons/<name>
// Returns a new GdkPixbuf (caller owns it), or nullptr on failure.
GdkPixbuf* silk_icon(const char* name);

// Convenience: make an icon image widget directly.
GtkWidget* silk_image(const char* name);

// Convenience: make a horizontal box with an icon + a text label.
GtkWidget* icon_label_box(const char* icon_name, const char* label_text, int spacing = 5);

} // namespace senkey
