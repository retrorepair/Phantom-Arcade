import React, { useState } from 'react';
import { DEPLOYABLE_FILES, DeployableFile } from '../data/deployableCode';
import { Download, Copy, Check, FileCode, HardDrive, Terminal, CheckCircle2 } from 'lucide-react';

export const ScriptExporter: React.FC = () => {
  const [selectedFile, setSelectedFile] = useState<DeployableFile>(DEPLOYABLE_FILES[0]);
  const [copied, setCopied] = useState<boolean>(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(selectedFile.code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleDownload = (file: DeployableFile) => {
    const blob = new Blob([file.code], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = file.filename;
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleDownloadAll = () => {
    DEPLOYABLE_FILES.forEach(file => {
      handleDownload(file);
    });
  };

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Title */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2">
            <span>Production Deployment Hub</span>
            <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-neutral-400">
              Ready-to-Deploy Files
            </span>
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Complete scripts, daemons, and system configurations ready to drop onto your PC and MiSTer FPGA SD card.
          </p>
        </div>

        <button
          onClick={handleDownloadAll}
          className="px-4 py-2 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded-lg text-xs flex items-center gap-1.5 transition-colors cursor-pointer self-start sm:self-auto"
        >
          <Download className="w-3.5 h-3.5" />
          <span>Download All 6 Files</span>
        </button>
      </div>

      {/* Compiled Binaries & Easy Installers Banner */}
      <div className="bg-neutral-900/90 border border-neutral-800 rounded-xl p-5 space-y-5">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
          <div>
            <div className="flex items-center gap-2">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
              <h3 className="text-base font-bold text-white tracking-tight">
                Official Main_MiSTer + Groovy_MiSTer End-User Release
              </h3>
              <span className="text-[10px] font-mono px-2 py-0.5 bg-amber-400/20 text-amber-300 border border-amber-400/30 rounded">
                Upstream 2026 Integrated
              </span>
            </div>
            <p className="text-xs text-neutral-400 mt-1 max-w-3xl">
              Official MiSTer Main binary merged with <code className="text-amber-300">psakhis/Groovy_MiSTer</code> support. Includes pre-compiled ARM binaries, Cyclone V FPGA RBF cores, native 15kHz CRT framebuffer graphical frontend, and all configuration files.
            </p>
          </div>
          
          {/* All-in-one ZIP button */}
          <a
            href="/downloads/phantom_arcade_mister_release.zip"
            download="phantom_arcade_mister_release.zip"
            className="px-4 py-2.5 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold text-xs rounded-lg flex items-center justify-center gap-2 shadow-lg shadow-amber-500/10 transition-all cursor-pointer whitespace-nowrap"
          >
            <Download className="w-4 h-4" />
            <span>Download Complete SD Card Pack (.zip · 5.7 MB)</span>
          </a>
        </div>

        {/* Primary Download Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-6 gap-3">
          {/* 1. Official Main MiSTer Binary */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">MiSTer_groovy</span>
                <span className="text-[9px] font-mono bg-purple-500/20 text-purple-300 border border-purple-500/30 px-1.5 py-0.5 rounded">
                  ARM GLIBC 2.28
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Official tested Release 0.7 binary (947 KB). Compatible with MiSTer rootfs glibc 2.28.
              </p>
            </div>
            <a
              href="/downloads/MiSTer_groovy"
              download="MiSTer_groovy"
              className="w-full py-1.5 px-2 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3 h-3 text-amber-400" />
              <span>MiSTer_groovy</span>
            </a>
          </div>

          {/* 2. FPGA RBF Core */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">Groovy.rbf</span>
                <span className="text-[9px] font-mono bg-amber-500/20 text-amber-300 border border-amber-500/30 px-1.5 py-0.5 rounded">
                  FPGA Core
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Official FPGA core (4.1 MB). Must be named <code className="text-amber-300">Groovy.rbf</code> for core hooks.
              </p>
            </div>
            <a
              href="/downloads/Groovy.rbf"
              download="Groovy.rbf"
              className="w-full py-1.5 px-2 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3 h-3 text-amber-400" />
              <span>Groovy.rbf</span>
            </a>
          </div>

          {/* 3. Native Framebuffer GUI */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">Framebuffer GUI</span>
                <span className="text-[9px] font-mono bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 px-1.5 py-0.5 rounded">
                  Static ARMv7
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Statically linked CRT frontend (375 KB). 0 external dependencies, double-buffered /dev/fb0.
              </p>
            </div>
            <a
              href="/downloads/phantom_mister_frontend"
              download="phantom_mister_frontend"
              className="w-full py-1.5 px-2 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3 h-3 text-amber-400" />
              <span>Frontend Binary</span>
            </a>
          </div>

          {/* 4. Windows Manager EXE */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">PhantomArcadeManager</span>
                <span className="text-[9px] font-mono bg-blue-500/20 text-blue-300 border border-blue-500/30 px-1.5 py-0.5 rounded">
                  Windows x64
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Win32 PC manager (2.7 MB). Zero-config bridge, ROM cataloging, and SwitchRes sync.
              </p>
            </div>
            <a
              href="/downloads/PhantomArcadeManager.exe"
              download="PhantomArcadeManager.exe"
              className="w-full py-1.5 px-2 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3 h-3" />
              <span>Manager (.exe)</span>
            </a>
          </div>

          {/* 5. C++ Git Patch */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">Groovy Main Patch</span>
                <span className="text-[9px] font-mono bg-rose-500/20 text-rose-300 border border-rose-500/30 px-1.5 py-0.5 rounded">
                  Source Diff
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Complete git diff (7.6 KB) merging Groovy_MiSTer into upstream official Main_MiSTer.
              </p>
            </div>
            <a
              href="/downloads/groovy_mister_official_main.patch"
              download="groovy_mister_official_main.patch"
              className="w-full py-1.5 px-2 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3 h-3 text-amber-400" />
              <span>Patch (.diff)</span>
            </a>
          </div>

          {/* 6. 1-Line Installer */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">1-Line Installer</span>
                <span className="text-[9px] font-mono bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 px-1.5 py-0.5 rounded">
                  Automated
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Runs on MiSTer CLI (F9 or SSH): installs cores, binaries, and sets up MiSTer.ini automatically.
              </p>
            </div>
            <button
              onClick={() => {
                navigator.clipboard.writeText('curl -k -sSL https://raw.githubusercontent.com/retrorepair/Phantom-Arcade/main/mister_client/install_mister.sh | bash');
                setCopied(true);
                setTimeout(() => setCopied(false), 2000);
              }}
              className="w-full py-1.5 px-2 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-[11px] rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              {copied ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3 text-amber-400" />}
              <span>{copied ? 'Copied to Clipboard!' : 'Copy Curl Command'}</span>
            </button>
          </div>
        </div>

        {/* How to Run Instructions */}
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 pt-3 border-t border-neutral-800/80">
          <div className="bg-neutral-950/60 p-4 rounded-lg border border-neutral-800/80">
            <h4 className="text-xs font-bold text-amber-400 flex items-center gap-2 mb-2">
              <span className="w-1.5 h-1.5 rounded-full bg-amber-400" />
              <span>Option A: Direct RBF Core Launch (Zero Scripts)</span>
            </h4>
            <ol className="text-[11px] text-neutral-300 space-y-1.5 list-decimal list-inside font-mono">
              <li>Copy <code className="text-amber-300">MiSTer_groovy</code> to <code className="text-neutral-200">/media/fat/MiSTer_groovy</code> (glibc 2.28 compatible)</li>
              <li>Copy <code className="text-amber-300">Groovy.rbf</code> to <code className="text-neutral-200">/media/fat/_Utility/Groovy.rbf</code> or <code className="text-neutral-200">/media/fat/_Arcade/Groovy.rbf</code></li>
              <li>Add to <code className="text-neutral-200">/media/fat/MiSTer.ini</code>: <code className="text-amber-300">[Groovy] main=MiSTer_groovy</code></li>
              <li>Launch <code className="text-neutral-200">Utility -&gt; Groovy</code> from the MiSTer OSD menu</li>
            </ol>
          </div>

          <div className="bg-neutral-950/60 p-4 rounded-lg border border-neutral-800/80">
            <h4 className="text-xs font-bold text-cyan-400 flex items-center gap-2 mb-2">
              <span className="w-1.5 h-1.5 rounded-full bg-cyan-400" />
              <span>Option B: Graphical Framebuffer CRT Launcher</span>
            </h4>
            <ol className="text-[11px] text-neutral-300 space-y-1.5 list-decimal list-inside font-mono">
              <li>Copy <code className="text-cyan-300">phantom_mister_frontend</code> & <code className="text-cyan-300">Phantom_Arcade.sh</code> to <code className="text-neutral-200">/media/fat/Scripts/</code></li>
              <li>Run <code className="text-neutral-200">PhantomArcadeManager.exe</code> on your Windows PC</li>
              <li>On MiSTer, select <code className="text-neutral-200">Scripts -&gt; Phantom_Arcade</code></li>
              <li>Enjoy the graphical CRT frontend with arcade stick game selection!</li>
            </ol>
          </div>
        </div>
      </div>

      {/* File Navigation & Code Viewer */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* File List (4 cols) */}
        <div className="lg:col-span-4 space-y-2">
          <div className="text-xs font-mono text-neutral-400 mb-2">Target Deployment Files:</div>
          {DEPLOYABLE_FILES.map((file) => {
            const isSelected = selectedFile.filename === file.filename;
            return (
              <button
                key={file.filename}
                onClick={() => setSelectedFile(file)}
                className={`w-full p-3 rounded-xl border text-left transition-colors cursor-pointer flex flex-col gap-1 ${
                  isSelected
                    ? 'bg-amber-500/10 border-amber-400/50 text-white shadow-sm'
                    : 'bg-neutral-900 border-neutral-800 text-neutral-400 hover:text-neutral-200 hover:border-neutral-700'
                }`}
              >
                <div className="flex items-center justify-between">
                  <span className="font-mono text-xs font-semibold text-white flex items-center gap-1.5">
                    <FileCode className="w-3.5 h-3.5 text-amber-400" />
                    <span>{file.filename}</span>
                  </span>
                  <span className="text-[10px] font-mono px-1.5 py-0.5 bg-neutral-950 rounded text-neutral-400">
                    {file.targetPlatform}
                  </span>
                </div>
                <div className="text-[11px] text-neutral-400 font-sans truncate">
                  {file.description}
                </div>
              </button>
            );
          })}

          {/* Quick Hardware Checklist */}
          <div className="p-4 bg-neutral-900/60 border border-neutral-800 rounded-xl space-y-2 mt-4">
            <div className="text-xs font-semibold text-neutral-200 flex items-center gap-1.5">
              <HardDrive className="w-3.5 h-3.5 text-emerald-400" />
              <span>MiSTer SD Card Layout</span>
            </div>
            <div className="text-[11px] text-neutral-400 space-y-1 font-mono">
              <div>📁 /media/fat/_Groovy/groovy.rbf</div>
              <div>📁 /media/fat/Scripts/phantom_arcade.sh</div>
              <div>📁 /media/fat/config/phantom.ini</div>
              <div>📁 /media/fat/config/games_catalog.json</div>
            </div>
          </div>
        </div>

        {/* Code Content & Action Bar (8 cols) */}
        <div className="lg:col-span-8 bg-neutral-900 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
          {/* File Header */}
          <div className="p-4 bg-neutral-900/90 border-b border-neutral-800 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
            <div>
              <div className="flex items-center gap-2">
                <span className="text-sm font-bold text-white font-mono">{selectedFile.filename}</span>
                <span className="text-[10px] font-mono px-2 py-0.5 bg-neutral-950 border border-neutral-800 rounded text-amber-400">
                  {selectedFile.language.toUpperCase()}
                </span>
              </div>
              <div className="text-xs text-neutral-400 font-mono mt-0.5">
                Target: {selectedFile.destinationPath}
              </div>
            </div>

            <div className="flex items-center gap-2">
              <button
                onClick={handleCopy}
                className="px-3 py-1.5 bg-neutral-800 hover:bg-neutral-700 text-neutral-200 text-xs font-mono rounded-lg transition-colors flex items-center gap-1.5 cursor-pointer"
              >
                {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                <span>{copied ? 'Copied' : 'Copy Code'}</span>
              </button>

              <button
                onClick={() => handleDownload(selectedFile)}
                className="px-3 py-1.5 bg-amber-400 hover:bg-amber-300 text-neutral-950 text-xs font-bold font-mono rounded-lg transition-colors flex items-center gap-1.5 cursor-pointer"
              >
                <Download className="w-3.5 h-3.5" />
                <span>Download</span>
              </button>
            </div>
          </div>

          {/* Code Viewer */}
          <pre className="flex-1 p-4 bg-neutral-950 font-mono text-xs text-neutral-300 overflow-x-auto max-h-[560px] custom-scrollbar leading-relaxed">
            {selectedFile.code}
          </pre>
        </div>

      </div>

      {/* Step by Step Setup Instructions */}
      <div className="p-6 bg-neutral-900/60 border border-neutral-800 rounded-xl space-y-4">
        <h3 className="text-base font-bold text-white flex items-center gap-2">
          <span>Quick 3-Step Setup Guide (Simplified)</span>
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4 text-xs">
          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-amber-400 font-mono">Step 1: Run Windows App</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              Download and run <code className="text-amber-300 font-mono">PhantomArcadeManager.exe</code> on your Windows PC.
              Click <strong className="text-neutral-200">Auto-Scan ROMs</strong>, then click <strong className="text-neutral-200">Start Background Daemon</strong>.
              No Python installation needed!
            </p>
          </div>

          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-emerald-400 font-mono">Step 2: Simple MiSTer Setup</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              Either run <code className="text-emerald-300 font-mono">curl -sSL http://&lt;PC_IP&gt;:8088/install | bash</code> in MiSTer F9 console,
              OR copy <code className="text-neutral-200 font-mono">Phantom_Arcade.sh</code> to <code className="text-neutral-200 font-mono">/media/fat/Scripts/</code>.
              It auto-discovers your PC and auto-downloads the Groovy core!
            </p>
          </div>

          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-blue-400 font-mono">Step 3: Play on 15kHz CRT</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              On your MiSTer cabinet, go to <strong>Scripts → Phantom_Arcade</strong>.
              Browse games with your arcade stick and press P1 Start. To return to MiSTer menu anytime, hold <strong>P1 Start + Coin</strong> for 1.2s!
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};
