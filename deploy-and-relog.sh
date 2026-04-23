#!/bin/sh
set -e

DIR="$(cd -- "$(dirname -- "$0")" && pwd)"
"$DIR/deploy.sh"

pkill -x dwm   2>/dev/null || true
pkill -x dwm_d 2>/dev/null || true

# Prefer restarting an active display manager; otherwise terminate the
# user session (works for startx/xinit setups).
DM=$(systemctl list-units --type=service --state=active --no-legend 2>/dev/null \
    | awk '{print $1}' \
    | grep -E '^(sddm|gdm|lightdm|lxdm|greetd|ly)\.service$' \
    | head -n1)

if [ -n "$DM" ]; then
    sudo systemctl restart "$DM"
else
    loginctl terminate-user "$USER" 2>/dev/null \
        || pkill -KILL -u "$USER"
fi
