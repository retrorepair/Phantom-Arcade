# Phantom Arcade — Engineering Handoff & Architecture Reference

## Executive Summary
Phantom Arcade is a decoupled client-server arcade streaming suite designed to run high-end arcade systems (GroovyMAME with Calamity SwitchRes, RetroArch, Sega Naomi, PS2, GameCube, Wii) from a host PC onto a MiSTer FPGA DE10-Nano streaming over Ethernet into the `Groovy.rbf` FPGA video sink core at native 15.7kHz CRT arcade fidelity.

---

## What Has Been Completed & Refined (Recent Prompts)
1. **Clean C++ Backend & Configuration Generation:**
   - Purged all legacy arguments (`-switchres 1`, `-resolution auto`, `-mister_ip`) from C++ config writer (`PhantomArcadeManager.cpp`) and React UI config templates.
   - Enforced clean, working command-line arguments: `"{rom_stem}" -video mister -skip_gameinfo -nokeepaspect`.
   - Recompiled `PhantomArcadeManager.exe` with MinGW-w64 (`x86_64-w64-mingw32-g++`) and updated release assets (`public/downloads/PhantomArcadeManager.exe` and `phantom_arcade_mister_release.zip`).

2. **MiSTer Frontend & Script Refinements (`phantom_mister_frontend.c` & `Phantom_Arcade.sh`):**
   - **Persistent Menu Loop (`Phantom_Arcade.sh`):** Wrapped execution so that when exiting a game or returning from MAME, the user is dropped directly back into the Phantom Arcade menu (`phantom_mister_frontend`) rather than the MiSTer menu or Groovy core bouncing ball screen.
   - **Last Selected Memory:** Added persistence for last selected game and category index so users land right back on their last played title.
   - **Marquee Title Scrolling:** Implemented smooth marquee scrolling for truncated game titles, complete with a 2-second pause at origin, 2-second pause at completion, and continuous loop.
   - **Consumer CRT TV Overscan Correction:** Added a 16px horizontal left inset to prevent right-side clipping on consumer RGB CRT monitors.
   - **Hold-to-Scroll Acceleration:** Added a 0.5-second (500ms) initial delay before scrolling starts, with dynamic acceleration the longer a button or stick is held down.
   - **Clean MAME ROM Naming:** Standardized MAME titles with clean naming conventions (Name, Version, Region), e.g., `Killer Instinct (v1.5, USA)`, `Street Fighter III: 3rd Strike (Euro 990512)`.

---

## File Manifest & Key Locations
* **`windows_setup/PhantomArcadeManager.cpp`**: Native Win32 C++17 configuration & daemon manager GUI source code.
* **`public/downloads/PhantomArcadeManager.exe`**: Pre-compiled Win32 executable available for direct download.
* **`public/downloads/phantom_arcade_mister_release.zip`**: Complete MiSTer release bundle (`Groovy.rbf`, `Phantom_Arcade.sh`, `phantom_mister_frontend`, `phantom.ini`).
* **`mister_client/phantom_mister_frontend.c`**: Native C framebuffer GUI source code for DE10-Nano ARM Linux.
* **`mister_release/Phantom_Arcade.sh`**: MiSTer launcher script.
* **`src/`**: Interactive React web application & simulator suite.

---

## Build & Verification Commands
- **Compile Web Applet:** `npm run build` / `compile_applet`
- **Compile Win32 C++ Manager:**
  ```bash
  x86_64-w64-mingw32-g++ -O2 -static -municode -mwindows windows_setup/PhantomArcadeManager.cpp -o windows_setup/PhantomArcadeManager.exe -lws2_32 -lshlwapi -lcomctl32 -lole32
  x86_64-w64-mingw32-strip windows_setup/PhantomArcadeManager.exe
  cp windows_setup/PhantomArcadeManager.exe public/downloads/PhantomArcadeManager.exe
  ```
