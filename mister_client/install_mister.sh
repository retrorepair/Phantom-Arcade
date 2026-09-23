#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — 1-Line MiSTer FPGA Auto-Installer
# Pulls directly from GitHub to eliminate any PC daemon/firewall dependency.
#
# Run on MiSTer (Press F9 for Linux CLI, or via SSH):
#   curl -k -sSL https://raw.githubusercontent.com/joelwhybrow/phantom-arcade-bridge/main/mister_client/install_mister.sh | bash
# =============================================================================

set -e

REPO_USER="joelwhybrow"
REPO_NAME="phantom-arcade-bridge"
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
    curl -k -L --connect-timeout 8 --max-time 60 \
      -o /media/fat/_Groovy/groovy.rbf \
      "https://raw.githubusercontent.com/MiSTer-devel/Groovy_MiSTer/main/releases/groovy.rbf" || {
        echo "[-] Note: Could not download groovy.rbf automatically."
        echo "    You can manually place groovy.rbf into /media/fat/_Groovy/"
    }
else
    echo "[2/4] Groovy_MiSTer core already present in /media/fat/_Groovy/groovy.rbf."
fi

# 2. Download Phantom_Arcade.sh from GitHub
echo "[3/4] Downloading latest Phantom_Arcade.sh launcher from GitHub..."
curl -k -L --connect-timeout 8 --max-time 30 \
  -o /media/fat/Scripts/Phantom_Arcade.sh \
  "${RAW_BASE}/mister_client/Phantom_Arcade.sh" || {
    echo "[-] Warning: Failed to download from GitHub main branch. Trying master..."
    curl -k -L --connect-timeout 8 --max-time 30 \
      -o /media/fat/Scripts/Phantom_Arcade.sh \
      "https://raw.githubusercontent.com/${REPO_USER}/${REPO_NAME}/master/mister_client/Phantom_Arcade.sh"
}

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
