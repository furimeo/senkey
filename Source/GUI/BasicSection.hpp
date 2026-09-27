// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include "Types.hpp"

namespace senkey {

class BasicSection {
private:
    GtkWidget* container;
    GtkWidget* charset_combo;
    GtkWidget* im_combo;
    GtkWidget* opt_ctrl_shift;
    GtkWidget* opt_alt_z;

public:
    BasicSection();
    ~BasicSection() = default;

    GtkWidget* get_widget() const { return container; }

    void load(const SenKeyConfig& cfg);
    void apply(SenKeyConfig& cfg);
};

} // namespace senkey
