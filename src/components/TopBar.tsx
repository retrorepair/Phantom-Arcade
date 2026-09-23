import React from 'react';
import { Terminal, Download, Play, Radio } from 'lucide-react';

interface TopBarProps {
  activeTab: string;
  setActiveTab: (tab: string) => void;
  serverActive: boolean;
  onQuickSimulate: () => void;
  onOpenExports: () => void;
}

export const TopBar: React.FC<TopBarProps> = ({
  activeTab,
  setActiveTab,
  serverActive,
  onQuickSimulate,
  onOpenExports
}) => {
  const navItems = [
    { id: 'simulator', label: 'Simulator' },
    { id: 'windows', label: 'Windows App' },
    { id: 'library', label: 'Library' },
    { id: 'network', label: 'Network Bus' },
    { id: 'modelines', label: 'Modelines' },
    { id: 'exports', label: 'Exports' }
  ];

  return (
    <header className="flex items-center justify-between px-6 py-3.5 border-b border-neutral-800 bg-neutral-950/95 backdrop-blur-md sticky top-0 z-50">
      {/* Zone 1: Single text element wordmark */}
      <div className="flex items-center gap-3">
        <a 
          href="#simulator" 
          onClick={(e) => { e.preventDefault(); setActiveTab('simulator'); }}
          className="text-lg font-bold tracking-tight text-white flex items-center gap-2 hover:text-amber-400 transition-colors"
        >
          <span className="w-2.5 h-2.5 rounded-full bg-amber-500 ring-4 ring-amber-500/20" />
          <span>Phantom Arcade</span>
        </a>
        <div className="hidden sm:flex items-center gap-1.5 text-xs text-neutral-500 font-mono">
          <span>·</span>
          <span>Groovy_MiSTer Bridge</span>
        </div>
      </div>

      {/* Zone 2: 4-6 clean text navigation links */}
      <nav className="hidden md:flex items-center gap-6 text-sm font-medium text-neutral-400">
        {navItems.map((item) => {
          const isActive = activeTab === item.id;
          return (
            <button
              key={item.id}
              onClick={() => setActiveTab(item.id)}
              className={`transition-colors relative py-1 text-sm ${
                isActive 
                  ? 'text-amber-400 font-semibold' 
                  : 'text-neutral-400 hover:text-neutral-200'
              }`}
            >
              {item.label}
              {isActive && (
                <span className="absolute bottom-0 left-0 right-0 h-0.5 bg-amber-400 rounded-full" />
              )}
            </button>
          );
        })}
      </nav>

      {/* Zone 3: 1-2 primary actions */}
      <div className="flex items-center gap-3">
        <div className="hidden lg:flex items-center gap-2 text-xs text-neutral-400 font-mono px-2.5 py-1 bg-neutral-900 border border-neutral-800 rounded-md">
          <span className={`w-2 h-2 rounded-full ${serverActive ? 'bg-emerald-400 animate-pulse' : 'bg-neutral-600'}`} />
          <span>UDP :2154</span>
          <span className="text-neutral-600">|</span>
          <span className="text-neutral-300">PC Daemon</span>
        </div>

        <button
          onClick={onQuickSimulate}
          className="flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-neutral-900 bg-amber-400 hover:bg-amber-300 rounded-md transition-colors whitespace-nowrap shadow-sm shadow-amber-500/10 cursor-pointer"
        >
          <Play className="w-3.5 h-3.5 fill-current" />
          <span>Test Launch</span>
        </button>

        <button
          onClick={onOpenExports}
          className="flex items-center gap-1.5 px-3 py-1.5 text-xs font-medium text-neutral-300 bg-neutral-900 hover:bg-neutral-800 border border-neutral-800 hover:border-neutral-700 rounded-md transition-colors whitespace-nowrap cursor-pointer"
        >
          <Download className="w-3.5 h-3.5" />
          <span className="hidden sm:inline">Get Scripts</span>
        </button>
      </div>
    </header>
  );
};
