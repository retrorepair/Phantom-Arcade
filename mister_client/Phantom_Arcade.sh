#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — 15.7kHz CRT Graphical Launcher
# Location: /media/fat/Scripts/Phantom_Arcade.sh
# =============================================================================

CONFIG_FILE="/media/fat/config/phantom.ini"
FRONTEND_BIN="/media/fat/Scripts/phantom_mister_frontend"
GROOVY_CORE_UTILITY="/media/fat/_Utility/Groovy.rbf"
GROOVY_CORE_ARCADE="/media/fat/_Arcade/Groovy.rbf"

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

# 2. If user has core installed, switch directly to Groovy RBF core
if [ -f "$GROOVY_CORE_ARCADE" ]; then
    echo "[+] Loading Groovy RBF core from Arcade folder..."
    echo "load_core $GROOVY_CORE_ARCADE" > /dev/MiSTer_cmd
    exit 0
elif [ -f "$GROOVY_CORE_UTILITY" ]; then
    echo "[+] Loading Groovy RBF core from Utility folder..."
    echo "load_core $GROOVY_CORE_UTILITY" > /dev/MiSTer_cmd
    exit 0
fi

echo "[!] Groovy.rbf core not found."
echo "Please copy Groovy.rbf into /media/fat/_Utility/ or /media/fat/_Arcade/"
exit 1
