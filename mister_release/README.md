# Phantom Arcade — MiSTer FPGA End-User Release

This release contains the complete set of tested files to run **Groovy_MiSTer** and **Phantom Arcade** with native 15.7kHz CRT output, zero input lag, and automatic modeline switching.

## Files in this Release

1. **`MiSTer_groovy`**
   - Official Groovy_MiSTer Main binary (tested Release 0.7, GLIBC 2.28 compatible for standard MiSTer Linux).
   - Place as `/media/fat/MiSTer_groovy` on your SD card root.
   - *Note*: Do NOT replace `/media/fat/MiSTer` directly; standard practice on MiSTer is placing it alongside as `MiSTer_groovy` and configuring `MiSTer.ini`.
2. **`MiSTer_groovy_XDP`**
   - High-performance AF_XDP zero-copy socket version for low latency. Requires `libelf.so.1` and `groovy_xdp_kern.o` in `/media/fat/`.
3. **`Groovy.rbf`**
   - Official FPGA core for Cyclone V DE10-Nano.
   - Place in `/media/fat/_Utility/Groovy.rbf` or `/media/fat/_Arcade/Groovy.rbf`.
   - **Crucial Core Naming Note**: MiSTer FPGA checks `strcasecmp(orig_name, "Groovy")`. The core filename MUST be `Groovy.rbf` for the internal UDP server to start.
4. **`phantom_mister_frontend`**
   - Native ARM Linux framebuffer GUI (`/dev/fb0`) statically compiled (zero glibc dependencies).
   - Place in `/media/fat/Scripts/phantom_mister_frontend`.
5. **`Phantom_Arcade.sh`**
   - MiSTer launcher script for the graphical UI.
   - Place in `/media/fat/Scripts/Phantom_Arcade.sh`.
6. **`MiSTer.ini`**
   - Configuration snippet ensuring `[Groovy]` launches `main=MiSTer_groovy`.

## Quick Setup Instructions

### Step 1: Copy Files to MiSTer SD Card
- Copy `MiSTer_groovy` to `/media/fat/MiSTer_groovy`
- Copy `Groovy.rbf` to `/media/fat/_Utility/Groovy.rbf` (or `/media/fat/_Arcade/Groovy.rbf`)
- Copy `phantom_mister_frontend` and `Phantom_Arcade.sh` to `/media/fat/Scripts/`

### Step 2: Configure `MiSTer.ini`
Add or merge this block into your `/media/fat/MiSTer.ini`:
```ini
[Groovy]
main=MiSTer_groovy
vga_scaler=0
composite_sync=1
ypbpr=0
direct_video=0
```

### Step 3: Launch
- Start `PhantomArcadeManager.exe` on your Windows PC.
- On your MiSTer, select **Scripts -> Phantom_Arcade** (for the CRT graphical game selector) or select **Utility -> Groovy** (for direct FPGA core streaming).
- The core initializes, establishes the UDP handshake with your PC, and switches to the game's native CRT modeline!
