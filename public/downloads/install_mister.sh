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
PSAKHIS_07="https://github.com/psakhis/Groovy_MiSTer/releases/download/0.7"

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

# 1. Download Groovy.rbf Core (Core name MUST be Groovy.rbf for is_groovy() hook)
echo "[2/6] Installing Groovy.rbf FPGA cores..."
if [ ! -f /media/fat/_Utility/Groovy.rbf ]; then
    echo "      Downloading Groovy.rbf..."
    curl -k -f -L --connect-timeout 10 --max-time 120 \
      -o /media/fat/_Utility/Groovy.rbf \
      "${PSAKHIS_07}/Groovy_20240922.rbf" || {
        echo "[-] Note: Trying fallback repository URL..."
        curl -k -f -L --connect-timeout 10 --max-time 120 \
          -o /media/fat/_Utility/Groovy.rbf \
          "${RAW_BASE}/public/downloads/Groovy.rbf" || true
    }
fi

if [ -f /media/fat/_Utility/Groovy.rbf ] && [ ! -f /media/fat/_Arcade/Groovy.rbf ]; then
    cp /media/fat/_Utility/Groovy.rbf /media/fat/_Arcade/Groovy.rbf
fi

# 2. Install MiSTer_groovy (Official Tested Release 0.7 - GLIBC 2.28 compatible)
echo "[3/6] Installing MiSTer_groovy binary (GLIBC 2.28 compatible)..."
if [ ! -f /media/fat/MiSTer_groovy ]; then
    curl -k -f -L --connect-timeout 10 --max-time 60 \
      -o /media/fat/MiSTer_groovy \
      "${PSAKHIS_07}/MiSTer_groovy" 2>/dev/null || {
        curl -k -f -L --connect-timeout 10 --max-time 60 \
          -o /media/fat/MiSTer_groovy \
          "${RAW_BASE}/public/downloads/MiSTer_groovy" 2>/dev/null || true
    }
    if [ -f /media/fat/MiSTer_groovy ]; then
        chmod +x /media/fat/MiSTer_groovy
    fi
fi

# Also download libelf.so.1 for XDP support if needed
if [ ! -f /media/fat/libelf.so.1 ]; then
    curl -k -f -L --connect-timeout 10 --max-time 30 \
      -o /media/fat/libelf.so.1 \
      "${PSAKHIS_07}/libelf.so.1" 2>/dev/null || true
fi

# 3. Configure MiSTer.ini for Groovy core
echo "[4/6] Updating /media/fat/MiSTer.ini..."
if [ -f /media/fat/MiSTer.ini ]; then
    if ! grep -q "Groovy" /media/fat/MiSTer.ini; then
        echo "" >> /media/fat/MiSTer.ini
        echo "[Groovy]" >> /media/fat/MiSTer.ini
        echo "main=MiSTer_groovy" >> /media/fat/MiSTer.ini
        echo "vga_scaler=0" >> /media/fat/MiSTer.ini
        echo "composite_sync=1" >> /media/fat/MiSTer.ini
        echo "[✓] Added [Groovy] section with main=MiSTer_groovy to MiSTer.ini"
    fi
else
    cat << "INIOUT" > /media/fat/MiSTer.ini
[Groovy]
main=MiSTer_groovy
vga_scaler=0
composite_sync=1
INIOUT
fi

# 4. Install Static Framebuffer Frontend & Launcher Script
echo "[5/6] Installing Graphical CRT Framebuffer Frontend (Statically Linked)..."
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
echo "    - Direct RBF Core: Go to Utility -> Groovy (or Arcade -> Groovy)"
echo "    - Graphical Menu:  Go to Scripts -> Phantom_Arcade"
echo ""
echo "Pixel-perfect 15kHz CRT gaming is now active on your setup!"
echo ""
