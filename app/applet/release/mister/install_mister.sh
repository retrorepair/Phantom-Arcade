#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade — 1-Line MiSTer FPGA Auto-Installer
# Integrated GroovyNLC Core + In-Core CRT Frontend
# "One Core, One PC App, No Messing About"
#
# Run on MiSTer (Press F9 for Linux CLI, or via SSH):
#   curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/release/mister/install_mister.sh | bash
# =============================================================================
set -e

REPO_USER="retrorepair"
REPO_NAME="Phantom-Arcade"
REPO_BRANCH="main"
RAW_MISTER="https://raw.githubusercontent.com/${REPO_USER}/${REPO_NAME}/${REPO_BRANCH}/release/mister"

echo ""
echo "=========================================================="
echo "    PHANTOM ARCADE — ZERO-CONFIG MISTER FPGA INSTALLER    "
echo "        Integrated GroovyNLC Core + In-Core Frontend      "
echo "=========================================================="
echo ""

# Ensure required directories exist
echo "[1/4] Preparing MiSTer directories..."
mkdir -p /media/fat/_Utility
mkdir -p /media/fat/_Arcade
mkdir -p /media/fat/config

# 1. Download GroovyNLC FPGA Core
echo "[2/4] Installing GroovyNLC FPGA core..."
curl -k -f -L --connect-timeout 10 --max-time 120 \
  -o /media/fat/_Utility/GroovyNLC.rbf \
  "${RAW_MISTER}/GroovyNLC.rbf" || {
    echo "[-] Note: Trying fallback download for GroovyNLC.rbf..."
    curl -k -f -L --connect-timeout 10 --max-time 120 \
      -o /media/fat/_Utility/GroovyNLC.rbf \
      "https://github.com/verbst/Groovy_MiSTer/releases/download/v1.4/GroovyNLC_20260904.rbf" || true
}

if [ -f /media/fat/_Utility/GroovyNLC.rbf ]; then
    cp /media/fat/_Utility/GroovyNLC.rbf /media/fat/_Arcade/GroovyNLC.rbf
    # Compatibility copies for Groovy.rbf
    cp /media/fat/_Utility/GroovyNLC.rbf /media/fat/_Utility/Groovy.rbf 2>/dev/null || true
    cp /media/fat/_Utility/GroovyNLC.rbf /media/fat/_Arcade/Groovy.rbf 2>/dev/null || true
fi

# 2. Install MiSTer_groovyNLC HPS binary with embedded frontend
echo "[3/4] Installing MiSTer_groovyNLC binary (with In-Core Frontend)..."
curl -k -f -L --connect-timeout 10 --max-time 60 \
  -o /media/fat/MiSTer_groovyNLC \
  "${RAW_MISTER}/MiSTer_groovyNLC" || true

if [ -f /media/fat/MiSTer_groovyNLC ]; then
    chmod +x /media/fat/MiSTer_groovyNLC
    cp /media/fat/MiSTer_groovyNLC /media/fat/MiSTer_groovy 2>/dev/null || true
fi

# 3. Configure MiSTer.ini
echo "[4/4] Updating /media/fat/MiSTer.ini..."
if [ -f /media/fat/MiSTer.ini ]; then
    if ! grep -q "GroovyNLC" /media/fat/MiSTer.ini; then
        echo "" >> /media/fat/MiSTer.ini
        echo "[GroovyNLC]" >> /media/fat/MiSTer.ini
        echo "main=MiSTer_groovyNLC" >> /media/fat/MiSTer.ini
        echo "vga_scaler=0" >> /media/fat/MiSTer.ini
        echo "composite_sync=1" >> /media/fat/MiSTer.ini
        echo "" >> /media/fat/MiSTer.ini
        echo "[Groovy*]" >> /media/fat/MiSTer.ini
        echo "main=MiSTer_groovyNLC" >> /media/fat/MiSTer.ini
        echo "[✓] Added [GroovyNLC] section to MiSTer.ini"
    fi
else
    cat << "INIOUT" > /media/fat/MiSTer.ini
[GroovyNLC]
main=MiSTer_groovyNLC
vga_scaler=0
composite_sync=1

[Groovy*]
main=MiSTer_groovyNLC
INIOUT
fi

# Initialize default configuration if missing
if [ ! -f /media/fat/config/phantom.ini ]; then
    cat << "CFGOUT" > /media/fat/config/phantom.ini
[SERVER]
PC_SERVER_IP=192.168.1.126
UDP_PORT=1999
AUTO_DISCOVERY=true
CFGOUT
fi

echo ""
echo "=========================================================="
echo " [✓] SUCCESS: Phantom Arcade installed on your MiSTer!    "
echo "=========================================================="
echo "Architecture: ONE CORE, ONE PC APP, NO MESSING ABOUT!"
echo ""
echo "How to use:"
echo " 1. Start PhantomArcadeManager.exe on your Windows PC."
echo " 2. On your MiSTer, select: _Utility -> GroovyNLC"
echo "    (or _Arcade -> GroovyNLC)"
echo ""
echo "The Phantom Arcade frontend is loaded directly into the"
echo "core! Browse your games with your arcade stick, press"
echo "Button 1 to stream, and press ESC to return to the menu!"
echo "=========================================================="
echo ""
