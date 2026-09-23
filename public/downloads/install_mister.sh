#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — 1-Line MiSTer FPGA Auto-Installer
# Repository: https://github.com/retrorepair/Phantom-Arcade
#
# Run on MiSTer (Press F9 for Linux CLI, or via SSH):
#   curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/mister_client/install_mister.sh | bash
# =============================================================================

set -e

REPO_USER="retrorepair"
REPO_NAME="Phantom-Arcade"
REPO_BRANCH="main"
RAW_BASE="https://raw.githubusercontent.com/${REPO_USER}/${REPO_NAME}/${REPO_BRANCH}"

echo ""
echo "=========================================================="
echo "    PHANTOM ARCADE — ZERO-CONFIG MISTER FPGA INSTALLER    "
echo "=========================================================="
echo ""

# Ensure required directories exist on SD card
echo "[1/4] Preparing MiSTer directories..."
mkdir -p /media/fat/_Groovy
mkdir -p /media/fat/Scripts
mkdir -p /media/fat/config

# 1. Download Groovy_MiSTer Core (groovy.rbf)
if [ ! -f /media/fat/_Groovy/groovy.rbf ]; then
    echo "[2/4] Downloading Groovy_MiSTer core (groovy.rbf)..."
    curl -k -f -L --connect-timeout 8 --max-time 60 \
      -o /media/fat/_Groovy/groovy.rbf \
      "https://raw.githubusercontent.com/MiSTer-devel/Groovy_MiSTer/main/releases/groovy.rbf" || {
        echo "[-] Note: Could not download groovy.rbf automatically."
        echo "    You can manually place groovy.rbf into /media/fat/_Groovy/"
    }
else
    echo "[2/4] Groovy_MiSTer core already present in /media/fat/_Groovy/groovy.rbf."
fi

# 2. Download Phantom_Arcade.sh from GitHub
echo "[3/4] Downloading latest Phantom_Arcade.sh launcher from GitHub (${RAW_BASE})..."
DOWNLOAD_OK=false

if curl -k -f -L --connect-timeout 8 --max-time 30 \
  -o /media/fat/Scripts/Phantom_Arcade.sh \
  "${RAW_BASE}/mister_client/Phantom_Arcade.sh" 2>/dev/null; then
    DOWNLOAD_OK=true
elif curl -k -f -L --connect-timeout 8 --max-time 30 \
  -o /media/fat/Scripts/Phantom_Arcade.sh \
  "https://raw.githubusercontent.com/${REPO_USER}/${REPO_NAME}/master/mister_client/Phantom_Arcade.sh" 2>/dev/null; then
    DOWNLOAD_OK=true
fi

if [ "$DOWNLOAD_OK" = false ]; then
    echo ""
    echo "=========================================================="
    echo " [!] ERROR: Could not download script from GitHub."
    echo "=========================================================="
    echo " Possible causes:"
    echo " 1. The repository https://github.com/${REPO_USER}/${REPO_NAME} is PRIVATE."
    echo "    GitHub returns '404: Not Found' to unauthenticated curl requests"
    echo "    for private repositories."
    echo ""
    echo "    To fix:"
    echo "    a) Set repo visibility to PUBLIC on GitHub (Settings -> Danger Zone),"
    echo "       OR"
    echo "    b) Copy Phantom_Arcade.sh directly from your PC to MiSTer via SCP:"
    echo "       scp mister_client/Phantom_Arcade.sh root@<MISTER_IP>:/media/fat/Scripts/"
    echo "=========================================================="
    exit 1
fi

chmod +x /media/fat/Scripts/Phantom_Arcade.sh

# 3. Initialize default configuration (Defaults to port 1999)
echo "[4/4] Setting default configuration..."
if [ ! -f /media/fat/config/phantom.ini ]; then
    cat <<EOF > /media/fat/config/phantom.ini
[SERVER]
PC_SERVER_IP=
UDP_PORT=1999
HTTP_PORT=8088
AUTO_DISCOVERY=true
EOF
fi

echo ""
echo "=========================================================="
echo " [✓] SUCCESS: Phantom Arcade installed on your MiSTer!    "
echo "=========================================================="
echo "How to launch:"
echo " 1. Turn on your PC and run PhantomArcadeManager.exe"
echo " 2. On your MiSTer, go to Main Menu -> Scripts"
echo " 3. Select 'Phantom_Arcade'"
echo ""
echo "The script will automatically discover your PC on UDP port 1999."
echo "Enjoy pixel-perfect 15kHz CRT gaming!"
echo ""
