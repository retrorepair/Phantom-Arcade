# Phantom Arcade — Groovy_MiSTer Client-Server Bridge & Arcade Launcher Suite

> **The Ultimate "Phantom" Arcade Experience**: Run demanding 3D & arcade systems (GroovyMAME, RetroArch SwitchRes, GameCube, Wii, Sega Naomi 2, Sega Model 2/3, PS2) from your MiSTer FPGA cabinet with zero perceived latency and 100% native 15kHz CRT arcade video fidelity.

---

## Overview

MiSTer FPGA provides cycle-accurate hardware simulation for retro consoles and arcade boards up to 32-bit/64-bit systems. However, heavier platforms and arcade sets like GroovyMAME with Calamity SwitchRes, RetroArch with CRT super resolutions, Nintendo GameCube, Nintendo Wii, Sega Naomi, and PS2 benefit immensely from high-speed PC server execution streamed directly into the MiSTer FPGA.

**Phantom Arcade** bridges this gap using a decoupled client-server architecture:
- **Client (MiSTer FPGA DE10-Nano)**: Runs a custom native Linux framebuffer frontend (`/dev/fb0` or `Phantom_Arcade.sh`) that displays an arcade cabinet game menu rendered at native 15kHz CRT resolution. When a game is selected, the client dispatches an ultra-fast UDP command over the local network and immediately boots Calamity's `groovy.rbf` FPGA core into frame-listening mode.
- **Server (Headless PC / Windows App)**: A silent background daemon or native C++ Windows setup app listens on UDP port `1999` (the official Calamity default, customizable in GUI). It matches the game ID, silently launches the emulator (GroovyMAME, RetroArch, Dolphin, Flycast, PCSX2) in batch fullscreen mode hooked into the Groovy_MiSTer video pipeline, and streams raw 15kHz frames over LAN into `groovy.rbf`.
- **Exit Hotkey Loop**: Holding `P1 Start + Coin` (or Select) for 1.2 seconds triggers the MiSTer input watcher to send a `KILL` UDP packet, terminating the PC emulator process cleanly and returning the cabinet to the native MiSTer menu.

To anyone playing on your arcade cabinet or CRT, **it feels like the MiSTer is running Street Fighter, Castlevania SOTN, Smash Melee, and Naomi Arcade natively**.

---

## Architecture Diagram

```
 ┌─────────────────────────────────────────────────────────────┐
 │                    MiSTer FPGA (DE10-Nano)                  │
 │                                                             │
 │   ARM Linux Side (Cortex-A9):                               │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  Phantom_Arcade.sh / Framebuffer CRT GUI (/dev/fb0) │   │
 │   │  - System filter tabs (GroovyMAME, RetroArch, etc.) │   │
 │   │  - Arcade stick input polling (/dev/input/event*)   │   │
 │   │  - Dispatches UDP LAUNCH packet to PC :1999         │   │
 │   │  - Hotkey supervisor: Start + Coin (1.2s) -> KILL   │   │
 │   └──────────────────────────┬──────────────────────────┘   │
 │                              │ echo "load_core groovy.rbf"  │
 │                              ▼                              │
 │   FPGA Cyclone V Fabric:                                    │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  groovy.rbf Core (Frame Buffer Sink)                │───┼───► 15kHz RGB / JAMMA
 │   │  Receives raw progressive/interlaced video frames   │   │     Arcade CRT Monitor
 │   └─────────────────────────────────────────────────────┘   │
 └──────────────────────────────▲──────────────────────────────┘
                                │
                    High-Speed Local LAN (Cat6)
         Command Bus (UDP :1999) | Video Stream (Groovy Protocol)
                                │
 ┌──────────────────────────────┴──────────────────────────────┐
 │                Headless PC Server (Host Machine)            │
 │                                                             │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  Phantom Arcade Daemon / C++ Windows Manager        │   │
 │   │  - Native Win32 GUI setup tool                      │   │
 │   │  - Listens on 0.0.0.0:1999 (configurable)           │   │
 │   │  - Serves games_catalog.json over HTTP :8088        │   │
 │   │  - Spawns & supervises emulator child processes     │   │
 │   └──────────────────────────┬──────────────────────────┘   │
 │                              │ Silent Batch Launch          │
 │                              ▼                              │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  Emulators (GroovyMAME, RetroArch, Dolphin, Flycast)│   │
 │   │  - Direct 15kHz Calamity Groovy_MiSTer SwitchRes    │   │
 │   │  - Pixel-perfect arcade modelines (CPS, NeoGeo,etc) │   │
 │   └─────────────────────────────────────────────────────┘   │
 └─────────────────────────────────────────────────────────────┘
```

---

## Repository Structure

```
├── README.md                      # Complete documentation & installation guide
├── windows_setup/                 # C++ Windows Setup & Management Application
│   ├── PhantomArcadeManager.cpp   # Native Win32 C++17 configuration & daemon GUI
│   ├── CMakeLists.txt             # Visual Studio / MinGW build script
│   └── build_windows.bat          # 1-click build script via MSVC cl.exe
├── mister_client/                 # MiSTer FPGA DE10-Nano Client Files
│   ├── phantom_mister_frontend.c  # Native C framebuffer (/dev/fb0) CRT GUI
│   ├── phantom_mister_menu.sh     # Lightweight bash menu alternative
│   ├── phantom_hotkey_daemon.sh   # Background arcade stick Start+Coin exit daemon
│   ├── phantom.ini                # Client configuration file (IP, scan rate)
│   └── Makefile                   # ARM cross-compilation Makefile for MiSTer
├── pc_daemon/                     # Python 3 Headless Server Daemon
│   ├── phantom_server.py          # Standalone UDP listener & process supervisor
│   ├── phantom_config.json        # Emulator path and command line bindings
│   ├── games_catalog.json         # ROM database and video mode catalog
│   └── phantom-bridge.service     # Systemd unit file for Linux gaming hosts
└── src/                           # Interactive Web Management Suite & Simulator
```

---

## The MiSTer CRT Frontend: Does It Look Exactly Like the Simulator?

**Yes.** Standard Linux terminal scripts output plain text. To match the interactive simulator, Phantom Arcade provides **`phantom_mister_frontend.c`**, a native C executable compiled for the DE10-Nano ARM processor.

### How It Works:
1. **Direct Framebuffer Access (`/dev/fb0`)**: Maps the MiSTer Linux video buffer directly into memory via `mmap()`.
2. **Authentic CRT Raster**: Configured for native 320x240 or 640x480 resolution at 15.7kHz horizontal refresh.
3. **Exact Visual Layout**:
   - Header with glowing amber branding (`PHANTOM ARCADE v1.2`) and scanning frequency (`15.7kHz · 240p`).
   - Horizontal console category tabs (`ALL`, `PS2`, `GAMECUBE`, `WII`, `NAOMI`, `MODEL2`).
   - Scrollable game list with high-contrast amber selection indicator (`▶`) and video mode tags.
   - P1 Start indicator and hold-to-quit progress animation.
4. **Arcade Stick Polling**: Directly reads joystick events from `/dev/input/js0` and Linux input events `/dev/input/event*`.

---

## Step-by-Step Setup Guide

### Phase 1: PC Server & Windows Setup (Compiled & Ready)

1. **Option A: Pre-Compiled Windows Desktop Application (Easiest)**:
   - Run the pre-compiled **`PhantomArcadeManager.exe`** (found in `windows_setup/bin/` or downloaded directly from the web interface).
   - Point the emulator pickers to your PCSX2, Dolphin, and Flycast executables.
   - Point the ROM folder paths (e.g. `C:\Games\PS2\`, `C:\Games\GameCube\`).
   - Click **"Auto-Scan ROMs"** to automatically populate `games_catalog.json`.
   - Click **"Start Background Daemon"**. It automatically listens on UDP `2154` and responds to MiSTer auto-discovery requests.

2. **Option B: Using Python 3**:
   - Run `python phantom_server.py`.

---

### Phase 2: MiSTer DE10-Nano Setup (Dead Simple)

Choose either of the two simplified installation methods:

#### Method 1: Direct GitHub 1-Line Installer (Requires Public Repository)
Pulls directly from GitHub's CDN to your MiSTer SD card. Does not require your PC server to be pre-running, and eliminates hangs caused by Windows Firewall:
1. On your MiSTer, press **F9** (or SSH into `root@mister.local`).
2. Run this single command:
   ```bash
   curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/mister_client/install_mister.sh | bash
   ```
   *Note: If your repository is set to **Private**, GitHub will reject unauthenticated curl calls with `404: Not Found` (resulting in `bash: line 1: 404:: command not found`). Either set the repository visibility to **Public** on GitHub (Settings → Danger Zone → Change to Public), or use Method 2 below.*

#### Method 2: SCP Direct Copy from PC (Works with Private or Public Repos)
If your repository is **Private** and you don't want to make it public, copy the files directly from your PC across your home network:
```bash
# From Windows PowerShell / Command Prompt / Terminal on your PC:
scp mister_client/Phantom_Arcade.sh root@<MISTER_IP>:/media/fat/Scripts/
```
Or open Windows File Explorer and navigate to your MiSTer Samba network share:
`\\<MISTER_IP>\media\fat\Scripts\` and drop `Phantom_Arcade.sh` there.

#### Method 3: Direct Script Download via curl (Public Repo)
If you only want the launcher script without running the installer:
```bash
curl -k -L -o /media/fat/Scripts/Phantom_Arcade.sh "https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/mister_client/Phantom_Arcade.sh" && chmod +x /media/fat/Scripts/Phantom_Arcade.sh
```

#### Method 4: Single-File Drop via SD Card
1. Copy **`Phantom_Arcade.sh`** into your MiSTer SD card at `/media/fat/Scripts/`.
2. Boot your MiSTer and select **Scripts → Phantom_Arcade**.
3. **No manual IP entry required!** The script automatically broadcasts a UDP probe across your local network (testing ports 1999 and 2154), discovers your running PC server, fetches your game library, and launches games.
4. If `groovy.rbf` is not found, the script will offer to download it automatically over your internet connection.

*(Optional)* For pixel-perfect direct framebuffer graphics on 15kHz CRT, you can also drop the pre-compiled ARM binary **`phantom_mister_frontend`** into `/media/fat/Scripts/`.
   - Open `/media/fat/config/phantom.ini` and set your PC IP:
     ```ini
     [SERVER]
     PC_SERVER_IP=192.168.1.100
     UDP_PORT=2154
     HTTP_PORT=8088
     ```
   - Copy `phantom_mister_frontend` (or `phantom_mister_menu.sh`) and `phantom_hotkey_daemon.sh` into `/media/fat/Scripts/`.
   - Make executable via SSH:
     ```bash
     chmod +x /media/fat/Scripts/phantom*
     ```

3. **Launch from MiSTer**:
   - On your arcade cabinet or CRT, open the MiSTer menu and select **Scripts → phantom_mister_frontend**.
   - Your CRT will switch into the native Phantom Arcade frontend!

---

### Phase 3: Arcade Stick Exit Hotkey

When playing games in the cabinet:
- Hold **`P1 Start + Coin`** (or Select) for **1.2 seconds**.
- The MiSTer hotkey daemon detects the combo, sends the `KILL` packet to the PC server, and reloads `/media/fat/menu.rbf`.
- The emulator is immediately terminated on the PC, clearing GPU memory for your next selection.

---

## Network Protocol Specification

All communication between MiSTer and PC is unencrypted, high-speed UDP/TCP designed for sub-millisecond local network execution.

| Packet | Transport | Direction | Payload Example | Purpose |
|---|---|---|---|---|
| `LAUNCH` | UDP :2154 | MiSTer ➔ PC | `LAUNCH:ps2_arcana_heart` | Requests instant emulator batch launch |
| `KILL` | UDP :2154 | MiSTer ➔ PC | `KILL:USER_HOTKEY` | Terminates active emulator subprocess |
| `PING` | UDP :2154 | MiSTer ➔ PC | `PING` | Verifies PC daemon availability |
| `ACK` | UDP :2154 | PC ➔ MiSTer | `ACK:LAUNCH:ps2_arcana_heart:SUCCESS` | Confirms execution status |
| `GET /catalog.json` | TCP :8088 | MiSTer ➔ PC | HTTP GET | Synchronizes games list and ROM metadata |

---

## CRT Modeline & 15kHz Safety

Standard arcade monitors (Nanao MS8, MS9, Sanwa 29E31S, Wells Gardner) operate at strict deflection frequencies. Phantom Arcade calculates and enforces SwitchRes-compliant video timings:
- **15kHz Standard**: $15.7\text{ kHz}$ horizontal, $59.94\text{ Hz}$ vertical (240p progressive / 480i interlaced).
- **24kHz Medium Resolution**: $24.8\text{ kHz}$ horizontal (Sega Model 2 / Model 3 at 496x384).
- **31kHz High Resolution**: $31.5\text{ kHz}$ horizontal (VGA / Naomi / Dreamcast at 640x480p).

---

## License

MIT License. Designed for the MiSTer FPGA and GroovyMAME retrogaming communities.
