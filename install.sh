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
    echo "--> Đang yêu cầu quyền quản trị (sudo) để cấu hình uinput và cài đặt..."
    if [ -t 0 ]; then
        exec sudo bash "$0" "$@"
    else
        exec sudo -S bash "$0" "$@"
    fi
fi

# Dừng sạch mọi tiến trình và dịch vụ cũ trước khi thực hiện cài đặt
echo "--> Dọn dẹp các tiến trình SenKey cũ đang chạy..."
systemctl stop senkey.service 2>/dev/null || true
if [ -n "$CURRENT_USER" ] && [ "$CURRENT_USER" != "root" ]; then
    USER_UID=$(id -u "$CURRENT_USER" 2>/dev/null || true)
    if [ -n "$USER_UID" ]; then
        XDG_RUNTIME_DIR="/run/user/$USER_UID" sudo -u "$CURRENT_USER" systemctl --user stop senkey.service 2>/dev/null || true
    fi
fi
pkill -9 -x senkey 2>/dev/null || true
pkill -9 -x senkey-gui 2>/dev/null || true
rm -f /tmp/senkey*.sock
rm -f "$USER_HOME/.config/systemd/user/senkey.service" 2>/dev/null || true

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

# Tự động cài đặt các thư viện hệ thống cần thiết nếu dùng Debian/Ubuntu (apt)
if command -v apt-get >/dev/null 2>&1; then
    MISSING_PKGS=""
    if ! dpkg -s libayatana-appindicator3-1 >/dev/null 2>&1 && ! dpkg -s libappindicator3-1 >/dev/null 2>&1; then
        MISSING_PKGS="$MISSING_PKGS libayatana-appindicator3-1"
    fi
    if ! dpkg -s libgtk-3-0 >/dev/null 2>&1; then
        MISSING_PKGS="$MISSING_PKGS libgtk-3-0"
    fi
    if ! dpkg -s libxkbcommon0 >/dev/null 2>&1; then
        MISSING_PKGS="$MISSING_PKGS libxkbcommon0"
    fi
    if ! dpkg -s acl >/dev/null 2>&1; then
        MISSING_PKGS="$MISSING_PKGS acl"
    fi
    if [ -n "$MISSING_PKGS" ]; then
        echo "--> Đang cài đặt thư viện hệ thống bổ trợ ($MISSING_PKGS)..."
        apt-get update -qq && apt-get install -y -qq $MISSING_PKGS 2>/dev/null || true
    fi
fi

# Cấu hình nạp kernel module uinput
echo "--> Thiết lập quyền thiết bị uinput..."
modprobe uinput 2>/dev/null || true
echo "uinput" > /etc/modules-load.d/uinput.conf

cat << 'EOF' > /etc/udev/rules.d/99-uinput.rules
KERNEL=="uinput", MODE="0660", GROUP="input", TAG+="uaccess", OPTIONS+="static_node=uinput"
SUBSYSTEM=="input", KERNEL=="event*", TAG+="uaccess"
EOF
udevadm control --reload-rules 2>/dev/null || true
udevadm trigger 2>/dev/null || true

# Áp dụng quyền chuẩn bảo mật 0660 root:input cho node /dev/uinput hiện tại
if [ -e /dev/uinput ]; then
    chown root:input /dev/uinput 2>/dev/null || true
    chmod 0660 /dev/uinput 2>/dev/null || true
fi

# Thêm người dùng vào nhóm input
if id -nG "$CURRENT_USER" | grep -qw "input"; then
    :
else
    usermod -aG input "$CURRENT_USER"
    echo "--> Đã thêm người dùng $CURRENT_USER vào nhóm input."
fi

# Cấp quyền tức thì qua POSIX ACL để người dùng gõ được ngay lập tức mà không cần đăng nhập lại
if command -v setfacl >/dev/null 2>&1; then
    if [ -e /dev/uinput ]; then
        setfacl -m u:"$CURRENT_USER":rw /dev/uinput 2>/dev/null || true
    fi
    if [ -d /dev/input ]; then
        setfacl -m u:"$CURRENT_USER":rw /dev/input/event* 2>/dev/null || true
    fi
    echo "--> Đã cấp quyền thiết bị tức thì cho $CURRENT_USER (POSIX ACL)."
fi

# Dừng triệt để tất cả tiến trình SenKey cũ trong bộ nhớ
echo "--> Dừng các tiến trình SenKey cũ đang chạy..."
systemctl stop senkey.service 2>/dev/null || true
pkill -9 -x senkey 2>/dev/null || true
pkill -9 -x senkey-gui 2>/dev/null || true
sleep 1

# Cài đặt tệp thực thi vào /usr/local/bin
# Cài đặt tệp thực thi vào /usr/local/bin
echo "--> Cài đặt tệp nhị phân vào /usr/local/bin..."
install -d /usr/local/bin
if [ -f "$SRC_DIR/senkey" ]; then
    install -m 755 "$SRC_DIR/senkey" /usr/local/bin/senkey
fi

# Dọn dẹp tệp thực thi phân tách cũ nếu có
rm -f /usr/local/bin/senkey-gui 2>/dev/null || true

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

# Tạo lối tắt ứng dụng .desktop trong Application Menu (mở bảng điều khiển GUI)
install -d /usr/share/applications
cat << 'EOF' > /usr/share/applications/senkey.desktop
[Desktop Entry]
Name=SenKey
GenericName=Bộ gõ tiếng Việt
Comment=Bảng điều khiển bộ gõ tiếng Việt SenKey
Exec=senkey -g
Icon=senkey
Terminal=false
Type=Application
Categories=Utility;Settings;
StartupNotify=false
EOF

# Cấu hình tự khởi động cùng phiên đăng nhập giao diện (Autostart)
install -d /etc/xdg/autostart
cat << 'EOF' > /etc/xdg/autostart/senkey.desktop
[Desktop Entry]
Name=SenKey
GenericName=Bộ gõ tiếng Việt
Comment=Bộ gõ tiếng Việt SenKey
Exec=senkey
Icon=senkey
Terminal=false
Type=Application
Categories=Utility;
StartupNotify=false
X-GNOME-Autostart-enabled=true
EOF

# Gỡ bỏ dịch vụ systemd cấp hệ thống cũ nếu có (SenKey giờ chạy trực tiếp trong user desktop session)
systemctl stop senkey.service 2>/dev/null || true
systemctl disable senkey.service 2>/dev/null || true
rm -f /etc/systemd/system/senkey.service 2>/dev/null || true
systemctl daemon-reload 2>/dev/null || true

# Cấu hình dịch vụ systemd cấp người dùng (user service)
install -d /etc/systemd/user
cat << 'EOF' > /etc/systemd/user/senkey.service
[Unit]
Description=SenKey Vietnamese Input Daemon
Documentation=https://github.com/furimeo/senkey
After=graphical-session.target
PartOf=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/local/bin/senkey
Restart=always
RestartSec=2

[Install]
WantedBy=default.target
EOF

if [ -n "$USER_HOME" ] && [ -d "$USER_HOME" ]; then
    AUTOSTART_DIR="$USER_HOME/.config/autostart"
    mkdir -p "$AUTOSTART_DIR"
    cp /etc/xdg/autostart/senkey.desktop "$AUTOSTART_DIR/senkey.desktop"
    rm -f "$USER_HOME/.config/systemd/user/senkey.service" 2>/dev/null || true
    chown -R "$CURRENT_USER:$CURRENT_USER" "$AUTOSTART_DIR" 2>/dev/null || true
fi

# Tự động khởi chạy SenKey cho người dùng nếu đang trong phiên đồ họa
if [ -n "$CURRENT_USER" ] && [ "$CURRENT_USER" != "root" ]; then
    USER_UID=$(id -u "$CURRENT_USER" 2>/dev/null || true)
    USER_RUNTIME="/run/user/$USER_UID"
    TARGET_DISPLAY="${DISPLAY:-:0}"
    TARGET_WAYLAND="${WAYLAND_DISPLAY}"
    
    if [ -d "$USER_RUNTIME" ]; then
        echo "--> Đang tự động khởi chạy SenKey (GUI & Khay hệ thống) cho $CURRENT_USER..."
        if command -v systemctl >/dev/null 2>&1 && sudo -u "$CURRENT_USER" XDG_RUNTIME_DIR="$USER_RUNTIME" systemctl --user is-system-running >/dev/null 2>&1; then
            sudo -u "$CURRENT_USER" XDG_RUNTIME_DIR="$USER_RUNTIME" systemctl --user daemon-reload 2>/dev/null || true
            sudo -u "$CURRENT_USER" XDG_RUNTIME_DIR="$USER_RUNTIME" systemctl --user enable --now senkey.service 2>/dev/null || true
        else
            sudo -u "$CURRENT_USER" \
                DISPLAY="$TARGET_DISPLAY" \
                WAYLAND_DISPLAY="$TARGET_WAYLAND" \
                XDG_RUNTIME_DIR="$USER_RUNTIME" \
                DBUS_SESSION_BUS_ADDRESS="unix:path=$USER_RUNTIME/bus" \
                nohup /usr/local/bin/senkey >/dev/null 2>&1 &
        fi
    fi
fi

# Dọn dẹp thư mục tạm nếu có
if [ -n "$TMP_DIR" ] && [ -d "$TMP_DIR" ]; then
    rm -rf "$TMP_DIR"
fi

echo "=========================================================="
echo " SenKey đã được cài đặt và kích hoạt thành công!"
echo " - Tệp nhị phân duy nhất: /usr/local/bin/senkey (Tích hợp GUI & Tray)"
echo " - Khay hệ thống (Tray): Tự động nạp cùng phiên Desktop"
echo " - Bảng điều khiển GUI: Gõ 'senkey' hoặc mở từ Application Menu"
echo " - Chuyển chế độ [V]/[E]: senkey -t (hoặc phím tắt Ctrl+Shift / Alt+Z)"
echo " - Kiểm tra trạng thái: senkey -s"
echo "=========================================================="
