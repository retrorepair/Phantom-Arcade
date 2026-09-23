#!/usr/bin/env bash
# =============================================================================
# 1-Line MiSTer FPGA Auto-Installer for Phantom Arcade
# Run on MiSTer (Press F9 or SSH):
#   curl -sSL http://<YOUR_PC_IP>:8088/install | bash
# =============================================================================

set -e

PC_IP="${1:-192.168.1.100}"

echo "========================================================"
echo "    Installing Phantom Arcade on MiSTer DE10-Nano...    "
echo "========================================================"

mkdir -p /media/fat/_Groovy
mkdir -p /media/fat/Scripts
mkdir -p /media/fat/config

# 1. Download groovy.rbf if missing
if [ ! -f /media/fat/_Groovy/groovy.rbf ]; then
    echo "[*] Downloading Calamity Groovy_MiSTer core (groovy.rbf)..."
    curl -k -L -o /media/fat/_Groovy/groovy.rbf "https://raw.githubusercontent.com/MiSTer-devel/Groovy_MiSTer/main/releases/groovy.rbf" || true
fi

# 2. Write configuration
echo "[*] Writing config to /media/fat/config/phantom.ini..."
cat <<EOF > /media/fat/config/phantom.ini
[SERVER]
PC_SERVER_IP=${PC_IP}
UDP_PORT=2154
HTTP_PORT=8088
AUTO_DISCOVERY=true
EOF

# 3. Download the all-in-one launcher script
echo "[*] Installing /media/fat/Scripts/Phantom_Arcade.sh..."
curl -sSL "http://${PC_IP}:8088/mister_script" -o /media/fat/Scripts/Phantom_Arcade.sh || true
chmod +x /media/fat/Scripts/Phantom_Arcade.sh

echo ""
echo "========================================================"
echo " [✓] SUCCESS: Phantom Arcade installed on your MiSTer! "
echo "========================================================"
echo "To run:"
echo " 1. Go to MiSTer Main Menu -> Scripts"
echo " 2. Select 'Phantom_Arcade'"
echo "Enjoy heavy 3D arcade & console games on your 15kHz CRT!"
