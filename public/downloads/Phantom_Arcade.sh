#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — 15.7kHz CRT Graphical Launcher
# Location: /media/fat/Scripts/Phantom_Arcade.sh
# =============================================================================

CONFIG_FILE="/media/fat/config/phantom.ini"
FRONTEND_BIN="/media/fat/Scripts/phantom_mister_frontend"
GROOVY_CORE="/media/fat/_Utility/Groovy.rbf"
PHANTOM_CORE="/media/fat/_Arcade/Phantom_Arcade.rbf"

# Load config if available
PC_IP=""
if [ -f "$CONFIG_FILE" ]; then
    PC_IP=$(grep -E "^PC_SERVER_IP=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d ' \r\n')
fi

# 1. If native framebuffer graphical frontend is present, launch it!
if [ -x "$FRONTEND_BIN" ]; then
    echo "[+] Launching Phantom Arcade Graphical Framebuffer CRT UI..."
    exec "$FRONTEND_BIN" "$PC_IP"
fi

# 2. If user has core installed, switch directly to RBF core
if [ -f "$PHANTOM_CORE" ]; then
    echo "[+] Loading Phantom Arcade RBF core..."
    echo "load_core $PHANTOM_CORE" > /dev/MiSTer_cmd
    exit 0
elif [ -f "$GROOVY_CORE" ]; then
    echo "[+] Loading Groovy_MiSTer RBF core..."
    echo "load_core $GROOVY_CORE" > /dev/MiSTer_cmd
    exit 0
fi

echo "[!] Phantom Arcade RBF core not found."
echo "Please copy Phantom_Arcade.rbf into /media/fat/_Arcade/ or Groovy.rbf into /media/fat/_Utility/"
exit 1
