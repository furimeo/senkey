// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <gtk/gtk.h>
#include <string>

namespace senkey {

// Nạp silk icon từ gói GResource nhúng sẵn trong binary.
// Đường dẫn tài nguyên: /senkey/icons/<tên_tệp>
// Trả về GdkPixbuf mới (bên gọi sở hữu đối tượng), hoặc nullptr nếu thất bại.
GdkPixbuf* silk_icon(const char* name);

// Tiện ích: tạo widget GtkImage trực tiếp từ icon.
GtkWidget* silk_image(const char* name);

// Tiện ích: tạo hộp ngang chứa icon và nhãn văn bản.
GtkWidget* icon_label_box(const char* icon_name, const char* label_text, int spacing = 5);

} // namespace senkey
