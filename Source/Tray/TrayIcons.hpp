// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#pragma once
#include <string>

namespace senkey {

class TrayIcons {
public:
    // Khởi tạo thư mục biểu tượng khay hệ thống và giải nén biểu tượng từ GResource nếu cần
    static std::string ensure_icon_directory();

    static const char* icon_name_vi() { return "senkey-v"; }
    static const char* icon_name_en() { return "senkey-e"; }
};

} // namespace senkey
