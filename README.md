# Phantom Arcade — Groovy_MiSTer Client-Server Bridge & Arcade Launcher Suite

> **The Ultimate "Phantom" Arcade Experience**: Run demanding 3D systems (PS2, GameCube, Wii, Sega Naomi 2, Sega Model 2/3, Saturn) from your MiSTer FPGA cabinet with zero perceived latency and 100% native 15kHz CRT arcade video fidelity.

---

## Overview

MiSTer FPGA provides cycle-accurate hardware simulation for retro consoles and arcade boards up to 32-bit/64-bit systems. However, heavier 3D platforms like the Sony PlayStation 2, Nintendo GameCube, Nintendo Wii, Sega Naomi 2, and Sega Model 2/3 exceed the logic capacity of the Cyclone V FPGA on the DE10-Nano.

**Phantom Arcade** bridges this gap using a decoupled client-server architecture:
- **Client (MiSTer FPGA DE10-Nano)**: Runs a custom native Linux framebuffer frontend (`/dev/fb0`) that displays an arcade cabinet game menu rendered at native 15kHz CRT resolution. When a game is selected, the client dispatches an ultra-fast UDP command over the local network and immediately boots Calamity's `groovy.rbf` FPGA core into frame-listening mode.
- **Server (Headless PC)**: A silent background daemon (or the native C++ Windows setup app) listens on UDP port `2154`. It matches the game ID, silently launches the emulator (PCSX2, Dolphin, Flycast, Model 2) in batch fullscreen mode hooked into the Groovy_MiSTer video pipeline, and streams raw 15kHz frames over LAN into `groovy.rbf`.
- **Exit Hotkey Loop**: Holding `P1 Start + Coin` (or Select) for 1.2 seconds triggers the MiSTer input watcher to send a `KILL` UDP packet, terminating the PC emulator process cleanly and returning the cabinet to the native MiSTer menu.

To anyone playing on your arcade cabinet or CRT, **it feels like the MiSTer is running Arcana Heart, Smash Melee, and Daytona USA natively**.

---

## Architecture Diagram

```
 ┌─────────────────────────────────────────────────────────────┐
 │                    MiSTer FPGA (DE10-Nano)                  │
 │                                                             │
 │   ARM Linux Side (Cortex-A9):                               │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  phantom_mister_frontend (Native /dev/fb0 CRT GUI)  │   │
 │   │  - System filter tabs (PS2, GameCube, Wii, Naomi)   │   │
 │   │  - Arcade stick input polling (/dev/input/event*)   │   │
 │   │  - Dispatches UDP LAUNCH packet to PC :2154         │   │
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
         Command Bus (UDP :2154) | Video Stream (Groovy Protocol)
                                │
 ┌──────────────────────────────┴──────────────────────────────┐
 │                Headless PC Server (Host Machine)            │
 │                                                             │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  Phantom Arcade Daemon / C++ Windows Manager        │   │
 │   │  - Sits silently in system tray / background        │   │
 │   │  - Listens on 0.0.0.0:2154                          │   │
 │   │  - Serves games_catalog.json over HTTP :8088        │   │
 │   │  - Spawns & supervises emulator child processes     │   │
 │   └──────────────────────────┬──────────────────────────┘   │
 │                              │ Silent Batch Launch          │
 │                              ▼                              │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  Emulators (PCSX2, Dolphin, Flycast, Model 2)       │   │
 │   │  - Direct3D 9 / Vulkan raw framebuffer hook         │   │
 │   │  - Calamity SwitchRes pixel clock & modelines       │   │
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

### Phase 1: PC Server & Windows Setup

1. **Option A: Using the C++ Windows Setup App**:
   - Open `windows_setup/` and run `build_windows.bat` (or use Visual Studio 2022).
   - Launch `PhantomArcadeManager.exe`.
   - Point the emulator directory pickers to your PCSX2, Dolphin, Flycast, and Model 2 executables.
   - Point your ROM folder paths (e.g. `C:\Games\PS2\`).
   - Click **"Auto-Scan ROMs"** to automatically populate your games catalog.
   - Enter your MiSTer IP address (e.g. `192.168.1.50`).
   - Click **"Start Daemon"**.

2. **Option B: Using Python 3**:
   - Install Python 3.8+ on your host PC.
   - Navigate to `pc_daemon/`.
   - Edit `phantom_config.json` to verify emulator executable paths.
   - Run:
     ```bash
     python phantom_server.py
     ```
   - Verify UDP listener is active on `0.0.0.0:2154` and HTTP catalog is on `0.0.0.0:8088`.

---

### Phase 2: MiSTer DE10-Nano Setup

1. **Install Groovy_MiSTer Core**:
   - Download the latest `groovy.rbf` from the official Groovy_MiSTer repository.
   - Place it on your MiSTer SD card at `/media/fat/_Groovy/groovy.rbf`.

2. **Deploy the Client Files**:
   - Copy `mister_client/phantom.ini` to `/media/fat/config/phantom.ini`.
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
