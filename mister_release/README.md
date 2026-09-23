# Phantom Arcade — MiSTer FPGA End-User Release

This release contains the complete set of files to run **Groovy_MiSTer** and **Phantom Arcade** with native 15.7kHz CRT output, zero input lag, and automatic modeline switching.

## Files in this Release

1. **`MiSTer` / `MiSTer_groovy`**
   - Official MiSTer Main binary compiled with upstream commit `aa271e4` + Groovy_MiSTer integration.
   - Place as `/media/fat/MiSTer_groovy` (or replace `/media/fat/MiSTer`).
2. **`MiSTer_groovy_XDP`**
   - High-performance kernel AF_XDP socket version for Linux kernels with XDP support.
3. **`Groovy.rbf`**
   - FPGA core for Cyclone V DE10-Nano.
   - Place in `/media/fat/_Utility/Groovy.rbf`.
4. **`Phantom_Arcade.rbf`**
   - FPGA core branded for Phantom Arcade.
   - Place in `/media/fat/_Arcade/Phantom_Arcade.rbf`.
5. **`phantom_mister_frontend`**
   - Native ARM Linux framebuffer GUI for 15kHz CRT arcade monitors.
   - Place in `/media/fat/Scripts/phantom_mister_frontend`.
6. **`Phantom_Arcade.sh`**
   - MiSTer script launcher to start the graphical UI directly from Scripts menu.
   - Place in `/media/fat/Scripts/Phantom_Arcade.sh`.
7. **`MiSTer.ini`**
   - Configuration snippet ensuring `[Groovy]` uses `main=MiSTer_groovy`.

## Quick Setup Instructions

### Option A: Direct RBF Core Launch (Recommended)
1. Copy `MiSTer_groovy` to the root of your SD card (`/media/fat/MiSTer_groovy`).
2. Copy `Groovy.rbf` to `/media/fat/_Utility/Groovy.rbf` and/or `Phantom_Arcade.rbf` to `/media/fat/_Arcade/Phantom_Arcade.rbf`.
3. Add the following to your `/media/fat/MiSTer.ini`:
   ```ini
   [Groovy]
   main=MiSTer_groovy
   ```
4. Start `PhantomArcadeManager.exe` on your PC.
5. On your MiSTer, select **Arcade -> Phantom_Arcade** (or **Utility -> Groovy**).
6. The FPGA core boots, connects via UDP:1999 to your PC, and displays your games in native 15.7kHz CRT resolution!

### Option B: Graphical Framebuffer Launcher
1. Copy `phantom_mister_frontend` and `Phantom_Arcade.sh` to `/media/fat/Scripts/`.
2. On your MiSTer, go to **Scripts -> Phantom_Arcade**.
3. Use your arcade joystick to browse games, view timings, and press Start (Button 1) to launch directly into the RBF core!
