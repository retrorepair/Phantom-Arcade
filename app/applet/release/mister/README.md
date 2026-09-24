# Phantom Arcade for MiSTer FPGA (GroovyNLC Integrated Core)

## "One Core, One PC App, No Messing About"

This release integrates the **Phantom Arcade Frontend** directly into the **GroovyNLC** FPGA core (`verbst/Groovy_MiSTer`), replacing the idle screensaver bouncing ball with a native 15.7kHz CRT arcade frontend interface.

---

### What Makes This Different?

1. **In-Core Frontend**:
   - The frontend is compiled straight into `MiSTer_groovyNLC` (ARM HPS).
   - When you launch `GroovyNLC.rbf`, it boots immediately into the Phantom Arcade menu at native 15.7kHz RGB CRT resolution.
   - No external bash scripts. No bouncing ball screen. No core switching back and forth.

2. **Full Controller & HPS Data Flow**:
   - In Menu: Your MiSTer arcade stick, gamepad, or keyboard directly navigates games with hold-to-scroll acceleration and 0.5s delay.
   - Long game titles smoothly marquee scroll (2-second pause, smooth scroll, 2-second pause, loop).
   - In Game: All HPS data (joysticks, buttons, analog sticks, rumble, keyboard) streams over UDP to the PC emulator seamlessly.
   - When Game Exits: Idle timeout detects emulator disconnect and immediately returns you to the Phantom Arcade menu at the exact same game!

3. **Clean MAME Naming**:
   - Formatted cleanly as `Name, Version, Region` (e.g. *Killer Instinct (v1.5, USA)*).

---

### Installation on MiSTer (1-Line Command)

Open an SSH session to your MiSTer (or press **F9** on keyboard) and run:

```bash
curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/release/mister/install_mister.sh | bash
```

### Manual Installation
1. Copy `GroovyNLC.rbf` to `/media/fat/_Utility/GroovyNLC.rbf`
2. Copy `MiSTer_groovyNLC` to `/media/fat/MiSTer_groovyNLC`
3. Add to `/media/fat/MiSTer.ini`:
   ```ini
   [GroovyNLC]
   main=MiSTer_groovyNLC
   vga_scaler=0
   composite_sync=1

   [Groovy*]
   main=MiSTer_groovyNLC
   ```
4. Copy `phantom.ini` to `/media/fat/config/phantom.ini` and set your `PC_SERVER_IP`.

---

### Files in this folder
- `GroovyNLC.rbf` — The GroovyNLC FPGA core bitstream (NLC compression, 15kHz CRT output)
- `MiSTer_groovyNLC` — The HPS binary with embedded Phantom Arcade frontend
- `Groovy.rbf` / `MiSTer_groovy` — Compatibility copies for standard naming
- `phantom.ini` — Configuration file
- `install_mister.sh` — Automated 1-line installation script
