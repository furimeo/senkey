// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <string>

namespace senkey {

class AboutDialog {
private:
    GtkWidget* parent;

    static GdkPixbuf* load_silk_icon(const std::string& name);

public:
    explicit AboutDialog(GtkWidget* parent_window = nullptr);
    ~AboutDialog() = default;

    void set_parent(GtkWidget* parent_window) { parent = parent_window; }
    void show();
};

} // namespace senkey
