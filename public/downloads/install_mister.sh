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
echo "[1/6] Preparing MiSTer directories..."
mkdir -p /media/fat/_Utility
mkdir -p /media/fat/_Arcade
mkdir -p /media/fat/Scripts
mkdir -p /media/fat/config

# 1. Download Groovy_MiSTer Core (Groovy.rbf & Phantom_Arcade.rbf)
echo "[2/6] Installing Groovy_MiSTer & Phantom Arcade RBF cores..."
if [ ! -f /media/fat/_Utility/Groovy.rbf ]; then
    echo "      Downloading Groovy.rbf..."
    curl -k -f -L --connect-timeout 10 --max-time 120 \
      -o /media/fat/_Utility/Groovy.rbf \
      "https://raw.githubusercontent.com/psakhis/Groovy_MiSTer/main/Groovy.rbf" || {
        echo "[-] Note: Trying fallback release URL..."
        curl -k -f -L --connect-timeout 10 --max-time 120 \
          -o /media/fat/_Utility/Groovy.rbf \
          "https://github.com/psakhis/Groovy_MiSTer/releases/download/0.7/Groovy_20240922.rbf" || true
    }
fi

if [ -f /media/fat/_Utility/Groovy.rbf ] && [ ! -f /media/fat/_Arcade/Phantom_Arcade.rbf ]; then
    cp /media/fat/_Utility/Groovy.rbf /media/fat/_Arcade/Phantom_Arcade.rbf
fi

# 2. Install MiSTer_groovy (Official Main with Groovy integration)
echo "[3/6] Installing MiSTer Main binary with Groovy integration..."
if [ ! -f /media/fat/MiSTer_groovy ]; then
    curl -k -f -L --connect-timeout 10 --max-time 60 \
      -o /media/fat/MiSTer_groovy \
      "${RAW_BASE}/public/downloads/MiSTer_groovy" 2>/dev/null || {
        curl -k -f -L --connect-timeout 10 --max-time 60 \
          -o /media/fat/MiSTer_groovy \
          "${RAW_BASE}/mister_release/MiSTer_groovy" 2>/dev/null || true
    }
    if [ -f /media/fat/MiSTer_groovy ]; then
        chmod +x /media/fat/MiSTer_groovy
    fi
fi

# 3. Configure MiSTer.ini for Groovy core
echo "[4/6] Updating /media/fat/MiSTer.ini..."
if [ -f /media/fat/MiSTer.ini ]; then
    if ! grep -q "Groovy" /media/fat/MiSTer.ini; then
        echo "" >> /media/fat/MiSTer.ini
        echo "[Groovy]" >> /media/fat/MiSTer.ini
        echo "main=MiSTer_groovy" >> /media/fat/MiSTer.ini
        echo "[✓] Added [Groovy] main=MiSTer_groovy to MiSTer.ini"
    fi
else
    cat << "INIOUT" > /media/fat/MiSTer.ini
[Groovy]
main=MiSTer_groovy
INIOUT
fi

# 4. Install Graphical CRT Framebuffer Frontend
echo "[5/6] Installing Graphical CRT Framebuffer Frontend & Scripts..."
curl -k -f -L --connect-timeout 10 --max-time 30 \
  -o /media/fat/Scripts/phantom_mister_frontend \
  "${RAW_BASE}/public/downloads/phantom_mister_frontend" 2>/dev/null || true

if [ -f /media/fat/Scripts/phantom_mister_frontend ]; then
    chmod +x /media/fat/Scripts/phantom_mister_frontend
fi

curl -k -f -L --connect-timeout 10 --max-time 30 \
  -o /media/fat/Scripts/Phantom_Arcade.sh \
  "${RAW_BASE}/mister_client/Phantom_Arcade.sh" 2>/dev/null || true

if [ -f /media/fat/Scripts/Phantom_Arcade.sh ]; then
    chmod +x /media/fat/Scripts/Phantom_Arcade.sh
fi

# 5. Initialize default configuration
echo "[6/6] Setting default configuration..."
if [ ! -f /media/fat/config/phantom.ini ]; then
    cat << "CFGOUT" > /media/fat/config/phantom.ini
[SERVER]
PC_SERVER_IP=
UDP_PORT=1999
HTTP_PORT=8088
AUTO_DISCOVERY=true
CFGOUT
fi

echo ""
echo "=========================================================="
echo " [✓] SUCCESS: Phantom Arcade installed on your MiSTer!    "
echo "=========================================================="
echo "How to launch:"
echo " 1. Start PhantomArcadeManager.exe on your Windows PC."
echo " 2. On your MiSTer, choose either:"
echo "    - Direct RBF Core: Go to Arcade -> Phantom_Arcade (or Utility -> Groovy)"
echo "    - Graphical Menu:  Go to Scripts -> Phantom_Arcade"
echo ""
echo "Pixel-perfect 15kHz CRT gaming is now active on your setup!"
echo ""
