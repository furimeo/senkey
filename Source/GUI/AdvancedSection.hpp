// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include "Types.hpp"

namespace senkey {

class AdvancedSection {
private:
    GtkWidget* revealer;
    GtkWidget* chk_modern;
    GtkWidget* chk_free;
    GtkWidget* chk_spell;
    GtkWidget* chk_macro;
    GtkWidget* spin_delay;

public:
    AdvancedSection();
    ~AdvancedSection() = default;

    GtkWidget* get_widget() const { return revealer; }

    void load(const SenKeyConfig& cfg);
    void apply(SenKeyConfig& cfg);

    void toggle();
    bool is_revealed() const;
};

} // namespace senkey
