import React, { useState } from 'react';
import { 
  AppWindow, 
  FolderOpen, 
  FileCode, 
  Play, 
  Square, 
  Save, 
  Check, 
  Search, 
  Wifi, 
  Download, 
  Copy,
  Terminal,
  AlertCircle,
  HardDrive
} from 'lucide-react';

export const WindowsSetupApp: React.FC = () => {
  const [viewMode, setViewMode] = useState<'app' | 'source'>('app');
  const [misterIp, setMisterIp] = useState('192.168.1.50');
  const [udpPort, setUdpPort] = useState('2154');
  const [pcsx2Exe, setPcsx2Exe] = useState('C:\\Emulators\\PCSX2\\pcsx2-qt.exe');
  const [ps2Roms, setPs2Roms] = useState('C:\\Games\\PS2');
  const [dolphinExe, setDolphinExe] = useState('C:\\Emulators\\Dolphin\\Dolphin.exe');
  const [gcRoms, setGcRoms] = useState('C:\\Games\\GameCube');
  const [flycastExe, setFlycastExe] = useState('C:\\Emulators\\Flycast\\flycast.exe');
  const [naomiRoms, setNaomiRoms] = useState('C:\\Games\\Arcade\\Naomi');

  const [statusText, setStatusText] = useState('Ready. Configure paths and click Auto-Scan ROMs.');
  const [isDaemonRunning, setIsDaemonRunning] = useState(false);
  const [scannedGames, setScannedGames] = useState<string[]>([
    '[PS2] Arcana Heart (ArcanaHeart.iso)',
    '[PS2] Capcom vs. SNK 2 (CapcomVsSNK2.iso)',
    '[GameCube] Super Smash Bros. Melee (SmashMelee.iso)',
    '[Wii] Tatsunoko vs. Capcom (TatsunokoVsCapcom.iso)',
    '[Naomi] Virtua Fighter 4 Final Tuned (vf4ft.zip)',
    '[Model 2] Daytona USA (daytona.zip)'
  ]);

  const [copiedCode, setCopiedCode] = useState(false);

  const handleScanRoms = () => {
    setStatusText('Scanning ROM directories: C:\\Games\\PS2, C:\\Games\\GameCube, C:\\Games\\Arcade...');
    setTimeout(() => {
      setScannedGames([
        '[PS2] Arcana Heart (ArcanaHeart.iso)',
        '[PS2] Capcom vs. SNK 2 (CapcomVsSNK2.iso)',
        '[PS2] Melty Blood Actress Again (MeltyBlood.iso)',
        '[PS2] Tekken 5 (Tekken5.iso)',
        '[GameCube] Super Smash Bros. Melee (SmashMelee.iso)',
        '[GameCube] F-Zero GX (FZeroGX.iso)',
        '[Wii] Tatsunoko vs. Capcom (TatsunokoVsCapcom.iso)',
        '[Naomi] Virtua Fighter 4 Final Tuned (vf4ft.zip)',
        '[Naomi] Marvel vs. Capcom 2 (mvsc2.zip)',
        '[Model 2] Daytona USA (daytona.zip)',
        '[Model 2] Sega Rally Championship (srally.zip)'
      ]);
      setStatusText('Scan complete: 11 ROM files matched and added to catalog.');
    }, 400);
  };

  const handleTestPing = () => {
    setStatusText(`Testing UDP handshake with MiSTer at ${misterIp}:${udpPort}...`);
    setTimeout(() => {
      setStatusText(`MiSTer Handshake Successful! (Ping: 0.42ms · Replied: PONG:PHANTOM_ONLINE)`);
    }, 350);
  };

  const handleSaveConfig = () => {
    const configData = {
      server: {
        listen_ip: '0.0.0.0',
        udp_port: parseInt(udpPort, 10),
        http_port: 8088,
        mister_client_ip: misterIp
      },
      emulators: {
        ps2: { exe: pcsx2Exe, args: '-batch -fullscreen -elf "{rom}"', roms_dir: ps2Roms },
        gamecube: { exe: dolphinExe, args: '-b -e "{rom}"', roms_dir: gcRoms },
        naomi: { exe: flycastExe, args: '"{rom}"', roms_dir: naomiRoms }
      }
    };
    const blob = new Blob([JSON.stringify(configData, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'phantom_config.json';
    a.click();
    URL.revokeObjectURL(url);
    setStatusText('Configuration saved to phantom_config.json');
  };

  const handleToggleDaemon = () => {
    if (!isDaemonRunning) {
      setIsDaemonRunning(true);
      setStatusText(`Daemon ACTIVE (Listening on UDP 0.0.0.0:${udpPort} · HTTP :8088)`);
    } else {
      setIsDaemonRunning(false);
      setStatusText('Daemon Stopped.');
    }
  };

  const cppSourceCode = `#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <winsock2.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Full native Win32 C++17 implementation located at /windows_setup/PhantomArcadeManager.cpp
// Compile with MSVC:
// cl.exe /std:c++17 /O2 /DUNICODE /D_UNICODE PhantomArcadeManager.cpp /link ws2_32.lib comctl32.lib shell32.lib user32.lib gdi32.lib`;

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Title */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2">
            <span>Windows Setup & Bridge Manager</span>
            <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-amber-400">
              Native C++17 Application
            </span>
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Easy-to-use desktop setup tool for configuring emulator paths, ROM directories, MiSTer IP, and running the background bridge.
          </p>
        </div>

        {/* View Mode Switcher */}
        <div className="flex items-center gap-1.5 p-1 bg-neutral-900 border border-neutral-800 rounded-lg text-xs font-mono">
          <button
            onClick={() => setViewMode('app')}
            className={`px-3 py-1.5 rounded transition-colors cursor-pointer flex items-center gap-1.5 ${
              viewMode === 'app'
                ? 'bg-neutral-800 text-white font-semibold shadow-xs'
                : 'text-neutral-400 hover:text-neutral-200'
            }`}
          >
            <AppWindow className="w-3.5 h-3.5" />
            <span>Interactive GUI</span>
          </button>
          <button
            onClick={() => setViewMode('source')}
            className={`px-3 py-1.5 rounded transition-colors cursor-pointer flex items-center gap-1.5 ${
              viewMode === 'source'
                ? 'bg-neutral-800 text-white font-semibold shadow-xs'
                : 'text-neutral-400 hover:text-neutral-200'
            }`}
          >
            <FileCode className="w-3.5 h-3.5" />
            <span>C++ Source</span>
          </button>
        </div>
      </div>

      {viewMode === 'app' ? (
        <div className="space-y-6">
          {/* Simulated Windows Native Window */}
          <div className="bg-neutral-900 border border-neutral-700/80 rounded-xl overflow-hidden shadow-2xl max-w-4xl mx-auto">
            
            {/* Windows Title Bar */}
            <div className="bg-neutral-800/90 border-b border-neutral-700 px-4 py-2 flex items-center justify-between select-none">
              <div className="flex items-center gap-2">
                <AppWindow className="w-4 h-4 text-amber-400" />
                <span className="text-xs font-semibold text-neutral-200">
                  Phantom Arcade - Groovy_MiSTer Windows Setup & Bridge Manager
                </span>
              </div>
              <div className="flex items-center gap-2">
                <div className="w-2.5 h-2.5 rounded-full bg-neutral-600" />
                <div className="w-2.5 h-2.5 rounded-full bg-neutral-600" />
                <div className="w-2.5 h-2.5 rounded-full bg-neutral-600" />
              </div>
            </div>

            {/* Window Content */}
            <div className="p-6 space-y-5 text-xs font-sans">
              
              {/* Row 1: Network Configuration */}
              <div className="p-4 bg-neutral-950/70 border border-neutral-800 rounded-lg space-y-3">
                <div className="text-xs font-bold text-white flex items-center gap-2">
                  <Wifi className="w-3.5 h-3.5 text-amber-400" />
                  <span>Network Connection (MiSTer DE10-Nano)</span>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-12 gap-3 items-center">
                  <div className="sm:col-span-4 text-neutral-400">MiSTer FPGA IP Address:</div>
                  <div className="sm:col-span-5">
                    <input
                      type="text"
                      value={misterIp}
                      onChange={(e) => setMisterIp(e.target.value)}
                      className="w-full bg-neutral-900 border border-neutral-700 rounded px-2.5 py-1.5 text-white font-mono text-xs focus:outline-none focus:border-amber-400"
                    />
                  </div>
                  <div className="sm:col-span-3">
                    <input
                      type="text"
                      value={udpPort}
                      onChange={(e) => setUdpPort(e.target.value)}
                      className="w-full bg-neutral-900 border border-neutral-700 rounded px-2.5 py-1.5 text-neutral-300 font-mono text-xs focus:outline-none focus:border-amber-400"
                      placeholder="UDP Port"
                    />
                  </div>
                </div>
              </div>

              {/* Row 2: PCSX2 Configuration */}
              <div className="space-y-2">
                <div className="font-semibold text-neutral-200">1. Sony PlayStation 2 (PCSX2 Groovy)</div>
                <div className="grid grid-cols-1 sm:grid-cols-12 gap-2 items-center">
                  <div className="sm:col-span-3 text-neutral-400 text-[11px]">Executable (.exe):</div>
                  <div className="sm:col-span-7">
                    <input
                      type="text"
                      value={pcsx2Exe}
                      onChange={(e) => setPcsx2Exe(e.target.value)}
                      className="w-full bg-neutral-950 border border-neutral-800 rounded px-2 py-1 text-neutral-300 font-mono text-[11px]"
                    />
                  </div>
                  <div className="sm:col-span-2">
                    <button 
                      onClick={() => setPcsx2Exe('C:\\Emulators\\PCSX2\\pcsx2-qt.exe')}
                      className="w-full py-1 bg-neutral-800 hover:bg-neutral-700 rounded text-neutral-300 text-[11px] cursor-pointer"
                    >
                      Browse...
                    </button>
                  </div>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-12 gap-2 items-center">
                  <div className="sm:col-span-3 text-neutral-400 text-[11px]">ROMs Folder (.iso/.chd):</div>
                  <div className="sm:col-span-7">
                    <input
                      type="text"
                      value={ps2Roms}
                      onChange={(e) => setPs2Roms(e.target.value)}
                      className="w-full bg-neutral-950 border border-neutral-800 rounded px-2 py-1 text-neutral-300 font-mono text-[11px]"
                    />
                  </div>
                  <div className="sm:col-span-2">
                    <button 
                      onClick={() => setPs2Roms('C:\\Games\\PS2')}
                      className="w-full py-1 bg-neutral-800 hover:bg-neutral-700 rounded text-neutral-300 text-[11px] cursor-pointer"
                    >
                      Browse...
                    </button>
                  </div>
                </div>
              </div>

              {/* Row 3: Dolphin Configuration */}
              <div className="space-y-2 pt-1 border-t border-neutral-800/60">
                <div className="font-semibold text-neutral-200">2. Nintendo GameCube & Wii (Dolphin CRT)</div>
                <div className="grid grid-cols-1 sm:grid-cols-12 gap-2 items-center">
                  <div className="sm:col-span-3 text-neutral-400 text-[11px]">Executable (.exe):</div>
                  <div className="sm:col-span-7">
                    <input
                      type="text"
                      value={dolphinExe}
                      onChange={(e) => setDolphinExe(e.target.value)}
                      className="w-full bg-neutral-950 border border-neutral-800 rounded px-2 py-1 text-neutral-300 font-mono text-[11px]"
                    />
                  </div>
                  <div className="sm:col-span-2">
                    <button 
                      onClick={() => setDolphinExe('C:\\Emulators\\Dolphin\\Dolphin.exe')}
                      className="w-full py-1 bg-neutral-800 hover:bg-neutral-700 rounded text-neutral-300 text-[11px] cursor-pointer"
                    >
                      Browse...
                    </button>
                  </div>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-12 gap-2 items-center">
                  <div className="sm:col-span-3 text-neutral-400 text-[11px]">ROMs Folder (.iso/.gcm):</div>
                  <div className="sm:col-span-7">
                    <input
                      type="text"
                      value={gcRoms}
                      onChange={(e) => setGcRoms(e.target.value)}
                      className="w-full bg-neutral-950 border border-neutral-800 rounded px-2 py-1 text-neutral-300 font-mono text-[11px]"
                    />
                  </div>
                  <div className="sm:col-span-2">
                    <button 
                      onClick={() => setGcRoms('C:\\Games\\GameCube')}
                      className="w-full py-1 bg-neutral-800 hover:bg-neutral-700 rounded text-neutral-300 text-[11px] cursor-pointer"
                    >
                      Browse...
                    </button>
                  </div>
                </div>
              </div>

              {/* Action Buttons Bar */}
              <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5 pt-3 border-t border-neutral-800">
                <button
                  onClick={handleScanRoms}
                  className="py-2 px-3 bg-neutral-800 hover:bg-neutral-700 border border-neutral-700 text-white rounded font-medium flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
                >
                  <Search className="w-3.5 h-3.5 text-amber-400" />
                  <span>1. Auto-Scan ROMs</span>
                </button>

                <button
                  onClick={handleTestPing}
                  className="py-2 px-3 bg-neutral-800 hover:bg-neutral-700 border border-neutral-700 text-white rounded font-medium flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
                >
                  <Wifi className="w-3.5 h-3.5 text-emerald-400" />
                  <span>2. Test MiSTer Ping</span>
                </button>

                <button
                  onClick={handleSaveConfig}
                  className="py-2 px-3 bg-neutral-800 hover:bg-neutral-700 border border-neutral-700 text-white rounded font-medium flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
                >
                  <Save className="w-3.5 h-3.5 text-blue-400" />
                  <span>3. Save Config</span>
                </button>

                <button
                  onClick={handleToggleDaemon}
                  className={`py-2 px-3 rounded font-bold flex items-center justify-center gap-1.5 transition-colors cursor-pointer ${
                    isDaemonRunning
                      ? 'bg-rose-600 hover:bg-rose-500 text-white'
                      : 'bg-amber-400 hover:bg-amber-300 text-neutral-950'
                  }`}
                >
                  {isDaemonRunning ? <Square className="w-3.5 h-3.5 fill-current" /> : <Play className="w-3.5 h-3.5 fill-current" />}
                  <span>{isDaemonRunning ? 'Stop Daemon' : 'Start Daemon'}</span>
                </button>
              </div>

              {/* Scanned Games List */}
              <div className="space-y-1.5">
                <div className="text-[11px] font-semibold text-neutral-300 flex items-center justify-between">
                  <span>Detected Game Catalog ({scannedGames.length} titles):</span>
                  <span className="text-[10px] text-neutral-500 font-mono">Will be written to games_catalog.json</span>
                </div>
                <div className="bg-neutral-950 border border-neutral-800 rounded p-2.5 h-32 overflow-y-auto font-mono text-[11px] text-neutral-300 space-y-1 custom-scrollbar">
                  {scannedGames.map((game, i) => (
                    <div key={i} className="hover:text-amber-300 cursor-default">
                      {game}
                    </div>
                  ))}
                </div>
              </div>

              {/* Status Bar */}
              <div className="bg-neutral-950 border border-neutral-800 px-3 py-1.5 rounded flex items-center justify-between text-[11px] font-mono">
                <span className="text-amber-300 truncate">{statusText}</span>
                <span className="text-neutral-500 shrink-0">Win32 GUI</span>
              </div>

            </div>
          </div>
        </div>
      ) : (
        /* Source Code View */
        <div className="space-y-4">
          <div className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl flex items-center justify-between">
            <div>
              <div className="text-sm font-bold text-white font-mono">windows_setup/PhantomArcadeManager.cpp</div>
              <div className="text-xs text-neutral-400">
                Native Win32 C++17 source code. Compile using Visual Studio or build_windows.bat.
              </div>
            </div>

            <div className="flex items-center gap-2">
              <button
                onClick={() => {
                  navigator.clipboard.writeText(cppSourceCode);
                  setCopiedCode(true);
                  setTimeout(() => setCopiedCode(false), 2000);
                }}
                className="px-3 py-1.5 bg-neutral-800 hover:bg-neutral-700 text-neutral-200 text-xs font-mono rounded-lg transition-colors flex items-center gap-1.5 cursor-pointer"
              >
                {copiedCode ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                <span>{copiedCode ? 'Copied' : 'Copy Source'}</span>
              </button>
            </div>
          </div>

          <div className="bg-neutral-900 border border-neutral-800 rounded-xl p-4 font-mono text-xs text-neutral-300 space-y-2">
            <div className="text-amber-400 font-bold">// Build Instructions (Windows Command Prompt):</div>
            <pre className="bg-neutral-950 p-3 rounded text-neutral-300 border border-neutral-800 overflow-x-auto">
{`cd windows_setup
build_windows.bat

# Or using CMake:
mkdir build && cd build
cmake ..
cmake --build . --config Release`}
            </pre>
          </div>
        </div>
      )}
    </div>
  );
};
