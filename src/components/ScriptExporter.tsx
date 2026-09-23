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
      <div className="bg-neutral-900/80 border border-neutral-800 rounded-xl p-5 space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-neutral-800/80 pb-3">
          <div>
            <h3 className="text-sm font-bold text-white flex items-center gap-2">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
              <span>Pre-Compiled Binaries & Instant Launchers</span>
            </h3>
            <p className="text-xs text-neutral-400 mt-0.5">
              No manual building or complex compiling required. Download the pre-built executables directly:
            </p>
          </div>
          <span className="text-[11px] font-mono text-neutral-400 px-2.5 py-1 bg-neutral-950 border border-neutral-800 rounded-md">
            Direct Downloads
          </span>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
          {/* Windows EXE */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">PhantomArcadeManager.exe</span>
                <span className="text-[9px] font-mono bg-blue-500/20 text-blue-400 border border-blue-500/30 px-1.5 py-0.5 rounded">
                  Windows x64
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Compiled native Win32 app (2.7 MB). GUI setup, ROM scanner & bridge daemon.
              </p>
            </div>
            <a
              href="/downloads/PhantomArcadeManager.exe"
              download="PhantomArcadeManager.exe"
              className="w-full py-1.5 px-3 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold text-xs rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3.5 h-3.5" />
              <span>Download (.exe)</span>
            </a>
          </div>

          {/* MiSTer Zero-Config Script */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">Phantom_Arcade.sh</span>
                <span className="text-[9px] font-mono bg-emerald-500/20 text-emerald-400 border border-emerald-500/30 px-1.5 py-0.5 rounded">
                  MiSTer Script
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                1-File Zero-Config Launcher. Auto-discovers PC IP on LAN via UDP broadcast!
              </p>
            </div>
            <a
              href="/downloads/Phantom_Arcade.sh"
              download="Phantom_Arcade.sh"
              className="w-full py-1.5 px-3 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-xs rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3.5 h-3.5" />
              <span>Download (.sh)</span>
            </a>
          </div>

          {/* MiSTer Compiled ARM Binary */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">phantom_mister_frontend</span>
                <span className="text-[9px] font-mono bg-purple-500/20 text-purple-400 border border-purple-500/30 px-1.5 py-0.5 rounded">
                  ARMv7 Linux
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Compiled framebuffer /dev/fb0 client for DE10-Nano (482 KB). 15kHz CRT native.
              </p>
            </div>
            <a
              href="/downloads/phantom_mister_frontend"
              download="phantom_mister_frontend"
              className="w-full py-1.5 px-3 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-xs rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Download className="w-3.5 h-3.5" />
              <span>Download (ARM ELF)</span>
            </a>
          </div>

          {/* 1-Line Web Installer */}
          <div className="bg-neutral-950 p-3.5 rounded-lg border border-neutral-800 flex flex-col justify-between gap-3 hover:border-amber-400/40 transition-colors">
            <div>
              <div className="flex items-center justify-between">
                <span className="text-xs font-bold text-white">GitHub 1-Line Installer</span>
                <span className="text-[9px] font-mono bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 px-1.5 py-0.5 rounded">
                  Zero-Config
                </span>
              </div>
              <p className="text-[11px] text-neutral-400 mt-1">
                Run on MiSTer (F9 or SSH): Pulls directly from GitHub CDN. No PC server or firewall setup needed to install!
              </p>
            </div>
            <button
              onClick={() => {
                navigator.clipboard.writeText('curl -k -sSL https://raw.githubusercontent.com/joelwhybrow/phantom-arcade-bridge/main/mister_client/install_mister.sh | bash');
                alert('Copied direct GitHub installer to clipboard:\ncurl -k -sSL https://raw.githubusercontent.com/joelwhybrow/phantom-arcade-bridge/main/mister_client/install_mister.sh | bash');
              }}
              className="w-full py-1.5 px-3 bg-neutral-800 hover:bg-neutral-700 text-neutral-100 font-bold text-xs rounded font-mono flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
            >
              <Copy className="w-3.5 h-3.5" />
              <span>Copy GitHub Curl</span>
            </button>
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
