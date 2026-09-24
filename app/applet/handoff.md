# Phantom Arcade — Engineering Handoff & Architecture Reference

## Executive Summary: "One Core, One PC App, No Messing About"

Phantom Arcade is an end-to-end arcade streaming suite integrated directly into the **GroovyNLC** core (`https://github.com/verbst/Groovy_MiSTer`), the high-performance evolution of GroovyMiSTer with NLC compression, dynamic PLL modelines, and ultra-low latency.

Instead of running external scripts, juggling bash loops, or getting stuck on a bouncing ball screensaver, the **Phantom Arcade Frontend is built directly into the GroovyNLC core (`MiSTer_groovyNLC`)**.

---

## Architecture: Integrated In-Core Design

```
 ┌─────────────────────────────────────────────────────────────┐
 │                    MiSTer FPGA (DE10-Nano)                  │
 │                                                             │
 │   Cyclone V FPGA Fabric:                                    │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  GroovyNLC.rbf FPGA Core                            │───┼───► 15.7kHz CRT RGB
 │   │  - Scans DDR Framebuffer directly to CRT Analog Out │   │     Zero Frame Lag
 │   └──────────────────────────▲──────────────────────────┘   │
 │                              │ Blit Framebuffer             │
 │   ARM Linux HPS (Cortex-A9): │                              │
 │   ┌──────────────────────────┴──────────────────────────┐   │
 │   │  MiSTer_groovyNLC (HPS Executable)                  │   │
 │   │  ├── In-Core Phantom Arcade Frontend UI             │   │
 │   │  │   - Boots on core load directly into CRT UI      │   │
 │   │  │   - 14px CRT overscan safe margin                │   │
 │   │  │   - Hold-to-scroll (0.5s delay + acceleration)   │   │
 │   │  │   - Long title marquee scroll (2s pause + loop)  │   │
 │   │  │   - Clean MAME titles (Name, Version, Region)    │   │
 │   │  │   - Game & category memory (phantom_state.ini)   │   │
 │   │  │   - Dispatches UDP LAUNCH:<game_id> to PC        │   │
 │   │  │                                                  │   │
 │   │  └── In-Game Streaming & HPS Data Flow Layer        │   │
 │   │      - Streams video (NLC/LZ4/Raw) straight to DDR  │   │
 │   │      - Passes joystick/buttons/analog over UDP 32101│   │
 │   │      - On game exit: seamlessly returns to menu!    │   │
 │   └──────────────────────────▲──────────────────────────┘   │
 └──────────────────────────────┼──────────────────────────────┘
                                │ UDP :32100 (Video/Audio)
               High-Speed LAN   │ UDP :32101 (Inputs/HPS Data)
                                │ UDP :1999 / 32105 (Launch Bus)
 ┌──────────────────────────────┴──────────────────────────────┐
 │                Windows PC (Host Machine)                    │
 │                                                             │
 │   ┌─────────────────────────────────────────────────────┐   │
 │   │  PhantomArcadeManager.exe (C++17 Win32 GUI)         │   │
 │   │  - Listens on UDP :1999 & :32105 for LAUNCH:<game>  │   │
 │   │  - Instant launch (0s delay, core is already loaded)│   │
 │   │  - Executes GroovyMAME with working command line:   │   │
 │   │    mame {rom} -video mister -skip_gameinfo          │   │
 │   │         -nokeepaspect                               │   │
 │   │  - On game exit, terminates and frees stream        │   │
 │   └─────────────────────────────────────────────────────┘   │
 └─────────────────────────────────────────────────────────────┘
```

---

## Release Directory Layout (`/release/`)

```
release/
├── pc/
│   └── PhantomArcadeManager.exe   (Win32 C++17 configuration & streaming daemon)
└── mister/
    ├── GroovyNLC.rbf             (Official GroovyNLC FPGA core bitstream)
    ├── MiSTer_groovyNLC          (ARM HPS binary with embedded Phantom Arcade frontend)
    ├── Groovy.rbf                (Compatibility copy)
    ├── MiSTer_groovy             (Compatibility copy)
    ├── phantom.ini               (MiSTer PC connection configuration)
    ├── install_mister.sh         (1-Line automated installer script)
    └── README.md                 (MiSTer quick start instructions)
```

---

## What Has Been Completed

1. **Integration into verbst/Groovy_MiSTer Core**:
   - Cloned and integrated into `https://github.com/verbst/Groovy_MiSTer` (GroovyNLC).
   - Built `phantom_frontend.h` and `phantom_frontend.cpp` into `support/groovy/` of `Main_MiSTer`.
   - Replaced screensaver bouncing logo with full 15.7kHz CRT arcade frontend interface rendered directly into the FPGA DDR framebuffer (`buffer[HEADER_OFFSET]`).
   - Connected `user_io` inputs: when not in-game, joysticks, buttons, analog sticks, and keyboard control the frontend menu with hold-to-scroll acceleration and 0.5s delay.
   - Long titles scroll with marquee animation (2s pause, smooth scroll, 2s pause, loop).
   - Clean MAME titles lookup (Name, Version, Region).
   - Persistent memory of last selected game and category in `/media/fat/config/phantom_state.ini`.

2. **HPS Data & Streaming Lifecycle**:
   - When a game is selected, `MiSTer_groovyNLC` sends `LAUNCH:<game_id>` over UDP to `PhantomArcadeManager.exe`.
   - `PhantomArcadeManager.exe` starts GroovyMAME with:
     `mame <game> -video mister -skip_gameinfo -nokeepaspect`
   - GroovyMAME connects to `MiSTer_groovyNLC` on port 32100 & 32101.
   - Video frames stream straight to the FPGA CRT display.
   - Controller and keyboard inputs (HPS data) pass over UDP 32101 to the PC emulator with zero lag.
   - When the user exits the game on PC (ESC or controller quit), `MiSTer_groovyNLC`'s idle timeout detects the end of the session and immediately returns to the Phantom Arcade menu in the core!

3. **Single Release Layout & 1-Line Installer**:
   - Clean root `/release/` structure with ONLY end-user files under `/release/pc/` and `/release/mister/`.
   - Updated `release/mister/install_mister.sh` to fetch all files directly from `release/mister/`.

---

## How to Install on MiSTer (1-Line Command)

Press **F9** on your MiSTer keyboard or open an SSH terminal and run:

```bash
curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/release/mister/install_mister.sh | bash
```

Once installed, just select **_Utility -> GroovyNLC** on your MiSTer to launch right into the Phantom Arcade CRT frontend!
