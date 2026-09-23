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

# 1. Launch native framebuffer graphical frontend in a persistent loop
if [ -x "$FRONTEND_BIN" ]; then
    while true; do
        setterm -cursor off > /dev/tty0 2>/dev/null
        stty -echo < /dev/tty0 2>/dev/null
        clear > /dev/tty0 2>/dev/null
        "$FRONTEND_BIN" "$PC_IP"
        RET=$?
        if [ $RET -eq 99 ]; then
            # User explicitly requested exit to MiSTer Menu
            echo "load_core /media/fat/menu.rbf" > /dev/MiSTer_cmd
            exit 0
        fi
        # When returning from Groovy.rbf / game exit, loop right back to Phantom Arcade menu!
        sleep 0.5
    done
fi

# 2. Fallback if frontend binary is missing
if [ -f "$GROOVY_CORE_ARCADE" ]; then
    echo "load_core $GROOVY_CORE_ARCADE" > /dev/MiSTer_cmd
    exit 0
elif [ -f "$GROOVY_CORE_UTILITY" ]; then
    echo "load_core $GROOVY_CORE_UTILITY" > /dev/MiSTer_cmd
    exit 0
fi

echo "[!] Groovy.rbf core not found."
exit 1
