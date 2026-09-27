#!/usr/bin/env bash
set -e

CURRENT_USER="${SUDO_USER:-$USER}"

if [ "$EUID" -ne 0 ]; then
    echo "Error: Must run with sudo: sudo ./install.sh"
    exit 1
fi

modprobe uinput || true
echo "uinput" > /etc/modules-load.d/uinput.conf

cat << 'EOF' > /etc/udev/rules.d/99-uinput.rules
KERNEL=="uinput", MODE="0660", GROUP="input", OPTIONS+="static_node=uinput"
EOF
udevadm control --reload-rules || true
udevadm trigger || true

usermod -aG input "$CURRENT_USER"

install -d /usr/local/bin
install -m 755 senkey /usr/local/bin/senkey
if [ -f "senkey-gui" ]; then
    install -m 755 senkey-gui /usr/local/bin/senkey-gui
fi

install -d /usr/share/applications
cat << 'EOF' > /usr/share/applications/senkey.desktop
[Desktop Entry]
Name=SenKey
Comment=Bộ gõ tiếng Việt độc lập cho Linux
Exec=senkey-gui
Icon=input-keyboard
Terminal=false
Type=Application
Categories=Utility;Settings;
EOF

USER_HOME=$(getent passwd "$CURRENT_USER" | cut -d: -f6)
SERVICE_DIR="$USER_HOME/.config/systemd/user"
mkdir -p "$SERVICE_DIR"
cat << 'EOF' > "$SERVICE_DIR/senkey.service"
[Unit]
Description=SenKey Vietnamese Input Daemon
After=graphical-session.target

[Service]
ExecStart=/usr/local/bin/senkey --daemon
Restart=on-failure
RestartSec=3

[Install]
WantedBy=default.target
EOF
chown -R "$CURRENT_USER:$CURRENT_USER" "$USER_HOME/.config/systemd"

echo "Installed senkey to /usr/local/bin/senkey"
if [ -f "senkey-gui" ]; then
    echo "Installed senkey-gui to /usr/local/bin/senkey-gui"
fi
echo "To enable user service: systemctl --user enable --now senkey.service"
