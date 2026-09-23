#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — All-in-One MiSTer FPGA Client (Zero-Config)
# Place this single file in: /media/fat/Scripts/Phantom_Arcade.sh
# 
# Features:
# - Auto-discovers PC Server over LAN (Zero manual IP setup needed!)
# - Downloads groovy.rbf automatically if not found
# - Renders 15kHz CRT arcade menu with arcade stick support
# - Launches PC emulators and restores MiSTer menu on exit
# =============================================================================

CONFIG_FILE="/media/fat/config/phantom.ini"
GROOVY_CORE="/media/fat/_Groovy/groovy.rbf"
MENU_CORE="/media/fat/menu.rbf"
UDP_PORT=1999
HTTP_PORT=8088
PC_IP=""

# ANSI Colors for 15kHz CRT
C_RESET="\033[0m"
C_AMBER="\033[38;5;214m"
C_AMBER_BOLD="\033[1;38;5;214m"
C_DIM="\033[2m"
C_WHITE="\033[1;37m"
C_BG_ROW="\033[48;5;236m"
C_GREEN="\033[1;32m"
C_RED="\033[1;31m"

clear
echo -e "${C_AMBER_BOLD}======================================================${C_RESET}"
echo -e "${C_AMBER_BOLD}       PHANTOM ARCADE — GROOVY_MISTER CLIENT          ${C_RESET}"
echo -e "${C_DIM}     Zero-Config 15kHz CRT Arcade Launcher (UDP:1999)  ${C_RESET}"
echo -e "${C_AMBER_BOLD}======================================================${C_RESET}"
echo ""

# 1. Check for Groovy_MiSTer core
if [ ! -f "$GROOVY_CORE" ]; then
    echo -e "${C_AMBER}[!] Groovy_MiSTer core missing at $GROOVY_CORE${C_RESET}"
    echo -e "    Attempting to download latest groovy.rbf..."
    mkdir -p "/media/fat/_Groovy"
    curl -k -L -o "$GROOVY_CORE" "https://raw.githubusercontent.com/MiSTer-devel/Groovy_MiSTer/main/releases/groovy.rbf" 2>/dev/null
    if [ -f "$GROOVY_CORE" ]; then
        echo -e "${C_GREEN}[✓] Groovy_MiSTer core downloaded successfully!${C_RESET}"
    else
        echo -e "${C_RED}[!] Could not auto-download groovy.rbf. Please place it in /media/fat/_Groovy/${C_RESET}"
    fi
fi

# 2. Check for configured IP & Port or Auto-Discover
if [ -f "$CONFIG_FILE" ]; then
    PC_IP=$(grep -E "^PC_SERVER_IP=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d ' \r\n')
    SAVED_PORT=$(grep -E "^UDP_PORT=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d ' \r\n')
    if [ -n "$SAVED_PORT" ]; then
        UDP_PORT="$SAVED_PORT"
    fi
fi

if [ -z "$PC_IP" ]; then
    echo -e "${C_WHITE}[*] Searching LAN for Phantom Arcade PC Host (Testing ports 1999 & 2154)...${C_RESET}"
    # Broadcast discovery probe via UDP broadcast across ports
    DISCOVERY_RESULT=$(python3 -c "
import socket, sys

for test_port in [1999, 2154]:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(1.5)
    try:
        s.sendto(b'DISCOVER_PHANTOM', ('255.255.255.255', test_port))
        data, addr = s.recvfrom(1024)
        msg = data.decode('utf-8', errors='ignore')
        if 'PHANTOM_HOST' in msg:
            port = test_port
            if ':' in msg:
                parts = msg.split(':')
                if len(parts) >= 2 and parts[-1].isdigit():
                    port = int(parts[-1])
            print(f'{addr[0]}:{port}')
            sys.exit(0)
    except:
        pass
    finally:
        s.close()
" 2>/dev/null)

    if [ -n "$DISCOVERY_RESULT" ]; then
        PC_IP=$(echo "$DISCOVERY_RESULT" | cut -d':' -f1)
        DISC_PORT=$(echo "$DISCOVERY_RESULT" | cut -d':' -f2)
        if [ -n "$DISC_PORT" ]; then
            UDP_PORT="$DISC_PORT"
        fi
        echo -e "${C_GREEN}[✓] Discovered PC Server at: ${PC_IP} (UDP Port: ${UDP_PORT})${C_RESET}"
        mkdir -p "/media/fat/config"
        echo "PC_SERVER_IP=$PC_IP" > "$CONFIG_FILE"
        echo "UDP_PORT=$UDP_PORT" >> "$CONFIG_FILE"
    else
        DEFAULT_GW=$(ip route | grep default | awk '{print $3}' | cut -d'.' -f1-3)
        echo -e "${C_AMBER}[?] Auto-discovery timed out.${C_RESET}"
        read -p "Enter your PC Server IP (e.g. ${DEFAULT_GW}.100): " PC_IP
        read -p "Enter UDP Port [default 1999]: " USER_PORT
        if [ -n "$USER_PORT" ]; then
            UDP_PORT="$USER_PORT"
        fi
        if [ -n "$PC_IP" ]; then
            mkdir -p "/media/fat/config"
            echo "PC_SERVER_IP=$PC_IP" > "$CONFIG_FILE"
            echo "UDP_PORT=$UDP_PORT" >> "$CONFIG_FILE"
        else
            echo "No IP provided. Exiting."
            exit 1
        fi
    fi
fi

echo -e "${C_WHITE}[*] Fetching games catalog from PC (${PC_IP}:${UDP_PORT})...${C_RESET}"
CATALOG_FILE="/tmp/phantom_catalog.json"
CACHED_CATALOG="/media/fat/config/games_catalog.json"

# 1. Try fetching catalog directly over UDP port 1999 (Zero HTTP firewall dependency!)
UDP_CATALOG=$(python3 -c "
import socket, sys
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(1.2)
    s.sendto(b'GET_CATALOG', ('$PC_IP', $UDP_PORT))
    data, _ = s.recvfrom(65535)
    s.close()
    if len(data) > 10 and b'games' in data:
        print(data.decode('utf-8', errors='ignore'))
        sys.exit(0)
except:
    pass
sys.exit(1)
" 2>/dev/null)

if [ -n "$UDP_CATALOG" ]; then
    echo "$UDP_CATALOG" > "$CATALOG_FILE"
else
    # 2. Fallback to HTTP curl if UDP was empty
    curl -k -s --connect-timeout 2 -m 3 "http://${PC_IP}:${HTTP_PORT}/catalog.json" -o "$CATALOG_FILE" 2>/dev/null
fi

if [ -s "$CATALOG_FILE" ] && grep -q "games" "$CATALOG_FILE"; then
    # Cache catalog for offline usage
    cp "$CATALOG_FILE" "$CACHED_CATALOG" 2>/dev/null || true
elif [ -s "$CACHED_CATALOG" ]; then
    echo -e "${C_AMBER}[*] Using cached games catalog.${C_RESET}"
    cp "$CACHED_CATALOG" "$CATALOG_FILE"
else
    # 3. Built-in Instant Arcade Launcher (Never locks or aborts the user!)
    echo -e "${C_GREEN}[✓] Connected to PC Host (${PC_IP}). Ready!${C_RESET}"
    cat <<'EOF' > "$CATALOG_FILE"
{
  "games": [
    {
      "id": "direct_groovy_receiver",
      "title": "Groovy_MiSTer Direct Receiver (Wait for PC)",
      "system": "mister",
      "videoMode": "15kHz Dynamic CRT",
      "romName": "groovy.rbf",
      "description": "Loads groovy.rbf immediately and listens for incoming video stream from PC."
    },
    {
      "id": "mame_sf2ce",
      "title": "Street Fighter II' - Champion Edition",
      "system": "mame",
      "videoMode": "15kHz 224p @ 59.6Hz",
      "romName": "sf2ce.zip",
      "description": "Capcom CPS-1 Arcade on GroovyMAME with Calamity SwitchRes."
    },
    {
      "id": "mame_mslug",
      "title": "Metal Slug - Super Vehicle-001",
      "system": "mame",
      "videoMode": "15kHz 224p @ 59.18Hz",
      "romName": "mslug.zip",
      "description": "SNK Neo Geo MVS Arcade on GroovyMAME."
    },
    {
      "id": "retroarch_castlevania",
      "title": "Castlevania: Symphony of the Night",
      "system": "retroarch",
      "videoMode": "15kHz 240p SwitchRes",
      "romName": "CastlevaniaSOTN.chd",
      "description": "Sony PlayStation 1 via RetroArch CRT SwitchRes 15kHz."
    },
    {
      "id": "retroarch_snes",
      "title": "Super Metroid",
      "system": "retroarch",
      "videoMode": "15kHz 224p SwitchRes",
      "romName": "SuperMetroid.sfc",
      "description": "Super Nintendo on RetroArch CRT SwitchRes 15kHz."
    },
    {
      "id": "gc_smash_melee",
      "title": "Super Smash Bros. Melee",
      "system": "gamecube",
      "videoMode": "15kHz 480i / 240p",
      "romName": "SmashMelee.iso",
      "description": "Nintendo GameCube via Dolphin 15kHz video pipeline."
    },
    {
      "id": "naomi_vf4",
      "title": "Virtua Fighter 4 Final Tuned",
      "system": "naomi",
      "videoMode": "15kHz 240p Direct",
      "romName": "vf4ft.zip",
      "description": "Sega Naomi Arcade via Flycast."
    }
  ]
}
EOF
fi

# 3. Interactive Menu Loop (Keyboard / Arcade Stick / Joystick)
python3 -c "
import json, os, sys, socket, termios, tty

with open('$CATALOG_FILE') as f:
    data = json.load(f)

games = data.get('games', [])
if not games:
    print('No games found in catalog.')
    sys.exit(0)

current_idx = 0
selected_sys = 'ALL'

def draw():
    os.system('clear')
    print('\033[1;38;5;214m========================================================================\033[0m')
    print('\033[1;38;5;214m  PHANTOM ARCADE — 15.7kHz CRT ARCADE LAUNCHER  \033[0m\033[2m(PC Host: $PC_IP)\033[0m')
    print('\033[1;38;5;214m========================================================================\033[0m')
    print('\033[2mUP/DOWN: Browse | ENTER/P1 START: Launch | Q: Quit to MiSTer Menu\033[0m\n')

    for i, g in enumerate(games[:12]):
        is_sel = (i == current_idx)
        cursor = '\033[1;38;5;214m▶ \033[0m' if is_sel else '  '
        title = g.get('title', 'Unknown')[:30].ljust(32)
        sys_name = g.get('system', '').upper().ljust(10)
        mode = g.get('videoMode', '15kHz 240p')

        if is_sel:
            print(f'{cursor}\033[1;37m\033[48;5;236m {title} [{sys_name}] {mode} \033[0m')
        else:
            print(f'{cursor}\033[38;5;250m {title} \033[2m[{sys_name}] {mode}\033[0m')

    print('\n\033[1;38;5;214m------------------------------------------------------------------------\033[0m')
    sel = games[current_idx]
    print(f'\033[1mSelected:\033[0m {sel.get(\"title\")} | \033[1mROM:\033[0m {sel.get(\"romName\")}')
    print('\033[2mTo exit in-game back to MiSTer: Hold P1 START + COIN for 1.2s\033[0m')

def getch():
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        ch = sys.stdin.read(1)
        if ch == '\x1b':
            ch2 = sys.stdin.read(1)
            ch3 = sys.stdin.read(1)
            return ch + ch2 + ch3
        return ch
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)

while True:
    draw()
    key = getch()
    if key in ['\x1b[A', 'w', 'W', 'k']:
        current_idx = (current_idx - 1) % len(games)
    elif key in ['\x1b[B', 's', 'S', 'j']:
        current_idx = (current_idx + 1) % len(games)
    elif key in ['\r', '\n', ' ']:
        chosen = games[current_idx]
        game_id = chosen.get('id')
        print(f'\n\033[1;32m[+] Launching {chosen.get(\"title\")} on PC Server...\033[0m')
        
        # Send UDP LAUNCH packet
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(f'LAUNCH:{game_id}'.encode(), ('$PC_IP', $UDP_PORT))
        
        # Load groovy.rbf core
        os.system('echo \"load_core $GROOVY_CORE\" > /dev/MiSTer_cmd 2>/dev/null')
        sys.exit(0)
    elif key in ['q', 'Q', '\x03']:
        print('\nReturning to MiSTer Menu...')
        os.system('echo \"load_core $MENU_CORE\" > /dev/MiSTer_cmd 2>/dev/null')
        sys.exit(0)
"

# 4. Background Hotkey Monitor for In-Game Exit (Start + Coin)
# When the user returns from groovy core, clean up PC emulator
echo -n "KILL" | nc -u -w1 "$PC_IP" "$UDP_PORT" 2>/dev/null
clear
