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
          <span>Quick 3-Step Setup Guide</span>
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4 text-xs">
          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-amber-400 font-mono">Step 1: PC Server Setup</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              Place <code className="text-neutral-200 font-mono">phantom_server.py</code> and <code className="text-neutral-200 font-mono">phantom_config.json</code> into <code className="text-neutral-200 font-mono">C:\PhantomArcade\</code>.
              Ensure Python 3 is installed. Run <code className="text-amber-300 font-mono">python phantom_server.py</code>.
            </p>
          </div>

          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-emerald-400 font-mono">Step 2: MiSTer SD Card Setup</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              Copy <code className="text-neutral-200 font-mono">mister_phantom_menu.sh</code> to <code className="text-neutral-200 font-mono">/media/fat/Scripts/</code> and make it executable (<code className="text-emerald-300 font-mono">chmod +x</code>).
              Ensure <code className="text-neutral-200 font-mono">groovy.rbf</code> is placed in <code className="text-neutral-200 font-mono">/media/fat/_Groovy/</code>.
            </p>
          </div>

          <div className="p-4 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
            <div className="font-bold text-blue-400 font-mono">Step 3: Play & Enjoy</div>
            <p className="text-neutral-400 leading-relaxed font-sans">
              On your MiSTer cabinet, navigate to <strong>Scripts → phantom_arcade</strong>.
              Pick <em>Arcana Heart</em>, <em>Smash Melee</em>, or <em>Virtua Fighter 4</em>.
              Play with zero perceptible lag on your 15kHz CRT arcade monitor!
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};
