#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026 Lê Hùng Quang Minh
#
# Kịch bản cài đặt và cập nhật SenKey từ bản dựng cục bộ hoặc GitHub Release

set -e

REPO="furimeo/senkey"
CURRENT_USER="${SUDO_USER:-$USER}"
USER_HOME=$(getent passwd "$CURRENT_USER" | cut -d: -f6)

if [ "$EUID" -ne 0 ]; then
    echo "Lỗi: Vui lòng chạy lệnh cài đặt với quyền quản trị: sudo ./install.sh"
    exit 1
fi

ACTION="${1:-install}"

download_latest_release() {
    echo "--> Đang tìm nạp phiên bản phát hành mới nhất từ GitHub ($REPO)..."
    LATEST_JSON=$(curl -sSL "https://api.github.com/repos/$REPO/releases/latest")
    TAG_NAME=$(echo "$LATEST_JSON" | grep -m1 '"tag_name":' | cut -d '"' -f 4)
    if [ -z "$TAG_NAME" ]; then
        echo "Lỗi: Không thể lấy thông tin phiên bản mới nhất từ GitHub."
        exit 1
    fi
    echo "--> Phiên bản mới nhất: $TAG_NAME"

    DOWNLOAD_URL=$(echo "$LATEST_JSON" | grep 'browser_download_url' | grep 'linux-amd64.tar.gz' | head -n1 | cut -d '"' -f 4)
    if [ -z "$DOWNLOAD_URL" ]; then
        # Thử với x86_64 nếu không có amd64
        DOWNLOAD_URL=$(echo "$LATEST_JSON" | grep 'browser_download_url' | grep 'tar.gz' | head -n1 | cut -d '"' -f 4)
    fi

    if [ -z "$DOWNLOAD_URL" ]; then
        echo "Lỗi: Không tìm thấy gói tar.gz phát hành cho Linux trong phiên bản $TAG_NAME."
        exit 1
    fi

    TMP_DIR=$(mktemp -d /tmp/senkey-install-XXXXXX)
    echo "--> Tải gói cài đặt: $DOWNLOAD_URL"
    curl -sSL "$DOWNLOAD_URL" -o "$TMP_DIR/senkey.tar.gz"

    echo "--> Đang giải nén..."
    tar -xzf "$TMP_DIR/senkey.tar.gz" -C "$TMP_DIR"
    SRC_DIR="$TMP_DIR"
}

# Xác định nguồn nhị phân: cục bộ hoặc tải về từ GitHub
SRC_DIR="."
if [ -f "./build/senkey" ]; then
    SRC_DIR="./build"
elif [ -f "./senkey" ]; then
    SRC_DIR="."
else
    # Không có binary cục bộ, tự động tải bản phát hành mới nhất
    download_latest_release
fi

if [ "$ACTION" = "update" ]; then
    echo "--> Đang cập nhật SenKey lên phiên bản mới nhất..."
    download_latest_release
fi

# Cấu hình nạp kernel module uinput
echo "--> Thiết lập quyền thiết bị uinput..."
modprobe uinput 2>/dev/null || true
echo "uinput" > /etc/modules-load.d/uinput.conf

cat << 'EOF' > /etc/udev/rules.d/99-uinput.rules
KERNEL=="uinput", MODE="0660", GROUP="input", OPTIONS+="static_node=uinput"
EOF
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

# Thêm người dùng vào nhóm input để có quyền phát phím
if id -nG "$CURRENT_USER" | grep -qw "input"; then
    :
else
    usermod -aG input "$CURRENT_USER"
    echo "--> Đã thêm $CURRENT_USER vào nhóm input (có thể cần đăng nhập lại để nhận nhóm mới)."
fi

# Cài đặt tệp thực thi vào /usr/local/bin
echo "--> Cài đặt tệp nhị phân vào /usr/local/bin..."
install -d /usr/local/bin
if [ -f "$SRC_DIR/senkey" ]; then
    install -m 755 "$SRC_DIR/senkey" /usr/local/bin/senkey
fi

if [ -f "$SRC_DIR/senkey-gui" ]; then
    install -m 755 "$SRC_DIR/senkey-gui" /usr/local/bin/senkey-gui
fi

# Cài đặt icon vào hệ thống nếu có
install -d /usr/share/icons/hicolor/48x48/apps
if [ -f "icons/senkey-v.png" ]; then
    install -m 644 "icons/senkey-v.png" /usr/share/icons/hicolor/48x48/apps/senkey.png
    install -m 644 "icons/senkey-v.png" /usr/share/icons/hicolor/48x48/apps/senkey-v.png
    install -m 644 "icons/senkey-e.png" /usr/share/icons/hicolor/48x48/apps/senkey-e.png
elif [ -f "$SRC_DIR/icons/senkey-v.png" ]; then
    install -m 644 "$SRC_DIR/icons/senkey-v.png" /usr/share/icons/hicolor/48x48/apps/senkey.png
    install -m 644 "$SRC_DIR/icons/senkey-v.png" /usr/share/icons/hicolor/48x48/apps/senkey-v.png
    install -m 644 "$SRC_DIR/icons/senkey-e.png" /usr/share/icons/hicolor/48x48/apps/senkey-e.png
fi

# Tạo lối tắt ứng dụng .desktop
install -d /usr/share/applications
cat << 'EOF' > /usr/share/applications/senkey.desktop
[Desktop Entry]
Name=SenKey
GenericName=Bộ gõ tiếng Việt
Comment=Bộ gõ tiếng Việt độc lập cho Linux
Exec=senkey-gui
Icon=senkey
Terminal=false
Type=Application
Categories=Utility;Settings;
StartupNotify=false
X-GNOME-Autostart-enabled=true
EOF

# Cấu hình dịch vụ systemd user service
SERVICE_DIR="$USER_HOME/.config/systemd/user"
mkdir -p "$SERVICE_DIR"
cat << 'EOF' > "$SERVICE_DIR/senkey.service"
[Unit]
Description=SenKey Vietnamese Input Daemon
After=graphical-session.target

[Service]
ExecStart=/usr/local/bin/senkey --daemon
Restart=on-failure
RestartSec=2

[Install]
WantedBy=default.target
EOF

chown -R "$CURRENT_USER:$CURRENT_USER" "$USER_HOME/.config/systemd" 2>/dev/null || true

# Dọn dẹp thư mục tạm nếu có
if [ -n "$TMP_DIR" ] && [ -d "$TMP_DIR" ]; then
    rm -rf "$TMP_DIR"
fi

echo "=========================================================="
echo " SenKey đã được cài đặt thành công!"
echo " - Tệp nhị phân: /usr/local/bin/senkey, /usr/local/bin/senkey-gui"
echo " - Khởi động bộ gõ: senkey (hoặc senkey-gui)"
echo " - Bật tự khởi động cùng phiên làm việc:"
echo "     systemctl --user enable --now senkey.service"
echo "=========================================================="
