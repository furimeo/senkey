// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2026 Lê Hùng Quang Minh

#include "TrayIcons.hpp"
#include <gio/gio.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>

namespace senkey {

static void extract_resource(const char* res_path, const std::string& dest_path) {
    if (std::filesystem::exists(dest_path)) return;
    GError* err = nullptr;
    GBytes* bytes = g_resources_lookup_data(res_path, G_RESOURCE_LOOKUP_FLAGS_NONE, &err);
    if (!bytes) {
        if (err) g_error_free(err);
        return;
    }
    gsize size = 0;
    const char* data = static_cast<const char*>(g_bytes_get_data(bytes, &size));
    std::ofstream out(dest_path, std::ios::binary);
    if (out.is_open()) {
        out.write(data, size);
        out.close();
    }
    g_bytes_unref(bytes);
}

std::string TrayIcons::ensure_icon_directory() {
    std::string base = "/tmp/senkey-" + std::to_string(getuid()) + "/icons";
    std::error_code ec;
    std::filesystem::create_directories(base, ec);

    extract_resource("/senkey/icons/senkey-v.png", base + "/senkey-v.png");
    extract_resource("/senkey/icons/senkey-e.png", base + "/senkey-e.png");

    return base;
}

} // namespace senkey
