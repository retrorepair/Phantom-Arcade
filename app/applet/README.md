# Phantom Arcade — GroovyNLC Core Integration & Arcade Launcher Suite

> **"One Core, One PC App, No Messing About"**
> An evolution of GroovyMiSTer with the frontend built directly into the core (`verbst/Groovy_MiSTer`), streaming PC emulators (GroovyMAME, RetroArch SwitchRes, Sega Naomi, PS2) to MiSTer FPGA at native 15.7kHz CRT arcade fidelity with full bidirectional HPS controller and video data flow.

---

## What is Phantom Arcade?

Phantom Arcade turns your MiSTer FPGA into a dedicated 15.7kHz CRT arcade frontend and video receiver.
By integrating directly with **GroovyNLC** ([verbst/Groovy_MiSTer](https://github.com/verbst/Groovy_MiSTer)):

1. **One Core on MiSTer**: You load `GroovyNLC.rbf` from your MiSTer menu. Instead of an empty bouncing ball screensaver, the core immediately presents the **Phantom Arcade Frontend** on your CRT monitor!
2. **One PC App on Windows**: Run `PhantomArcadeManager.exe` on your PC. It configures GroovyMAME with `-video mister -skip_gameinfo -nokeepaspect`, scans ROMs, and listens for game launches.
3. **Seamless Game Launching**: Select any game using your arcade stick or gamepad. The core triggers the PC over UDP. GroovyMAME connects to the running core and starts streaming immediately — no FPGA reconfiguration delay, no black screen.
4. **HPS Data Passes Straight Through**: While playing, your MiSTer controllers (arcade stick buttons, D-pad, analog stick, keyboard) pass seamlessly over UDP to the PC emulator.
5. **Clean Exit Back to Menu**: When you exit the game (ESC or controller quit), GroovyNLC's idle timeout detects disconnect and **immediately restores the Phantom Arcade frontend menu**, remembering your exact game and category!

---

## Release Structure (`/release/`)

The repository root keeps end-user files organized strictly under `/release/`:

```
release/
├── pc/
│   └── PhantomArcadeManager.exe   (Win32 C++17 configuration & streaming daemon)
└── mister/
    ├── GroovyNLC.rbf             (GroovyNLC FPGA core bitstream)
    ├── MiSTer_groovyNLC          (ARM HPS binary with embedded Phantom Arcade frontend)
    ├── Groovy.rbf                (Compatibility core copy)
    ├── MiSTer_groovy             (Compatibility binary copy)
    ├── phantom.ini               (Configuration file)
    ├── install_mister.sh         (1-Line automated installer script)
    └── README.md                 (MiSTer quick start instructions)
```

---

## 1-Line MiSTer Installation

Open an SSH session to your MiSTer (or press **F9** for the console) and run:

```bash
curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/release/mister/install_mister.sh | bash
```

### Manual Setup
1. Copy `release/mister/GroovyNLC.rbf` to `/media/fat/_Utility/GroovyNLC.rbf`
2. Copy `release/mister/MiSTer_groovyNLC` to `/media/fat/MiSTer_groovyNLC`
3. Add to `/media/fat/MiSTer.ini`:
   ```ini
   [GroovyNLC]
   main=MiSTer_groovyNLC
   vga_scaler=0
   composite_sync=1

   [Groovy*]
   main=MiSTer_groovyNLC
   ```
4. Copy `release/mister/phantom.ini` to `/media/fat/config/phantom.ini` with your PC's IP address.
5. On your PC, launch `PhantomArcadeManager.exe`.
6. On your MiSTer, select **_Utility -> GroovyNLC**!

---

## Frontend Highlights

- **Native 15.7kHz CRT Output**: Direct 256x240 RGB888 rendering into DDR framebuffer scanned out by the FPGA.
- **CRT TV Overscan Inset**: 14px horizontal and 6px vertical safety insets to prevent right-side clipping on consumer CRT TVs.
- **Clean MAME Naming**: Parsed as `Name, Version, Region` (e.g. *Killer Instinct (v1.5, USA)*).
- **Marquee Title Scrolling**: 2-second pause at origin, smooth scroll across, 2-second pause at completion, continuous loop.
- **Hold-to-Scroll Acceleration**: 0.5-second initial delay to prevent accidental skips, with smooth acceleration the longer a direction is held.
- **Game & Category Memory**: Remembers last selected game and category across reboots and game exits.
