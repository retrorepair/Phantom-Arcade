import React, { useState, useEffect, useRef } from 'react';
import { GameItem, MiSTerState, PacketLog } from '../types';
import { 
  Tv, 
  Server, 
  Cpu, 
  Play, 
  Square, 
  ArrowUp, 
  ArrowDown, 
  Zap, 
  RefreshCw, 
  Radio, 
  Terminal as TerminalIcon,
  Maximize2,
  CheckCircle2,
  Clock,
  Sparkles
} from 'lucide-react';

interface ArcadeSimulatorProps {
  games: GameItem[];
  selectedGame: GameItem;
  setSelectedGame: (game: GameItem) => void;
  misterState: MiSTerState;
  setMisterState: (state: MiSTerState) => void;
  onSendPacket: (type: 'LAUNCH' | 'KILL' | 'PING', payload: string) => void;
  serverActive: boolean;
}

export const ArcadeSimulator: React.FC<ArcadeSimulatorProps> = ({
  games,
  selectedGame,
  setSelectedGame,
  misterState,
  setMisterState,
  onSendPacket,
  serverActive
}) => {
  const [activeSystemFilter, setActiveSystemFilter] = useState<string>('all');
  const [scanlinesEnabled, setScanlinesEnabled] = useState<boolean>(true);
  const [activePid, setActivePid] = useState<number | null>(null);
  const [serverLogs, setServerLogs] = useState<string[]>([
    '[21:04:12] [INFO] Phantom Arcade Bridge Daemon v1.2 initialized',
    '[21:04:12] [INFO] Listening for UDP commands on 0.0.0.0:2154',
    '[21:04:12] [INFO] HTTP Catalog service online at http://0.0.0.0:8088/catalog.json',
    '[21:04:15] [INFO] MiSTer FPGA client detected at 192.168.1.50 (DE10-Nano)',
    '[21:04:15] [INFO] Handshake acknowledged. Groovy_MiSTer video streamer ready.'
  ]);

  // Hotkey hold timer for arcade stick Start+Coin exit
  const [holdProgress, setHoldProgress] = useState<number>(0);
  const [isHoldingQuit, setIsHoldingQuit] = useState<boolean>(false);
  const holdIntervalRef = useRef<NodeJS.Timeout | null>(null);

  const filteredGames = activeSystemFilter === 'all' 
    ? games 
    : games.filter(g => g.system === activeSystemFilter);

  const addLog = (msg: string) => {
    const time = new Date().toTimeString().split(' ')[0];
    setServerLogs(prev => [...prev.slice(-30), `[${time}] ${msg}`]);
  };

  // Launch procedure
  const handleLaunch = (game: GameItem) => {
    if (misterState === 'STREAMING_ACTIVE') {
      handleKill();
      setTimeout(() => proceedLaunch(game), 600);
    } else {
      proceedLaunch(game);
    }
  };

  const proceedLaunch = (game: GameItem) => {
    setSelectedGame(game);
    setMisterState('SENDING_UDP');
    onSendPacket('LAUNCH', game.id);
    addLog(`[UDP RECV] Received LAUNCH command for ID: '${game.id}'`);

    // Simulate network transit & core loading
    setTimeout(() => {
      setMisterState('LOADING_CORE');
      addLog(`[PC PROC] Spawning ${game.emulatorId} with ROM: "${game.romPath}"`);
      const newPid = Math.floor(10000 + Math.random() * 89999);
      setActivePid(newPid);
      addLog(`[PC PROC] Process started successfully with PID ${newPid}`);
      addLog(`[VIDEO] Hooked Groovy_MiSTer pipeline -> SwitchRes mode: ${game.resolution} @ 15kHz`);

      setTimeout(() => {
        setMisterState('STREAMING_ACTIVE');
        addLog(`[STREAM] Streaming raw 15kHz frames over LAN to MiSTer (groovy.rbf active)`);
      }, 700);
    }, 400);
  };

  // Kill procedure (Arcade stick Quit combo)
  const handleKill = () => {
    if (misterState !== 'STREAMING_ACTIVE' && misterState !== 'LOADING_CORE') return;
    
    setMisterState('KILLING_PROCESS');
    onSendPacket('KILL', 'USER_HOTKEY');
    addLog(`[UDP RECV] Received KILL command from MiSTer arcade stick combo`);
    
    setTimeout(() => {
      addLog(`[PC PROC] Terminating emulator process PID ${activePid}...`);
      setActivePid(null);
      addLog(`[PC PROC] Process closed cleanly. VRAM flushed.`);
      addLog(`[MISTER] Restoring standard MiSTer menu core (menu.rbf)`);
      setMisterState('MENU_IDLE');
    }, 500);
  };

  // Handle Quit button press & hold
  const startHoldQuit = () => {
    if (misterState !== 'STREAMING_ACTIVE') return;
    setIsHoldingQuit(true);
    setHoldProgress(0);

    const startTime = Date.now();
    const duration = 1200; // 1.2 seconds hold

    holdIntervalRef.current = setInterval(() => {
      const elapsed = Date.now() - startTime;
      const progress = Math.min(100, Math.floor((elapsed / duration) * 100));
      setHoldProgress(progress);

      if (elapsed >= duration) {
        clearInterval(holdIntervalRef.current!);
        holdIntervalRef.current = null;
        setIsHoldingQuit(false);
        setHoldProgress(0);
        handleKill();
      }
    }, 30);
  };

  const cancelHoldQuit = () => {
    if (holdIntervalRef.current) {
      clearInterval(holdIntervalRef.current);
      holdIntervalRef.current = null;
    }
    setIsHoldingQuit(false);
    setHoldProgress(0);
  };

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Header bar */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2.5">
            <span>Interactive Bridge Simulator</span>
            <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-neutral-400">
              Live Testbed
            </span>
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Test the complete client-server cycle: MiSTer menu selection, UDP dispatch, groovy.rbf core swap, and arcade hotkey termination.
          </p>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={() => setScanlinesEnabled(!scanlinesEnabled)}
            className={`px-3 py-1.5 text-xs font-mono rounded-md border transition-colors cursor-pointer ${
              scanlinesEnabled 
                ? 'bg-amber-400/10 text-amber-300 border-amber-400/30' 
                : 'bg-neutral-900 text-neutral-400 border-neutral-800 hover:text-neutral-200'
            }`}
          >
            CRT Scanlines: {scanlinesEnabled ? 'ON' : 'OFF'}
          </button>

          {misterState === 'STREAMING_ACTIVE' && (
            <button
              onClick={handleKill}
              className="flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold text-rose-300 bg-rose-950/60 border border-rose-800/80 hover:bg-rose-900/60 rounded-md transition-colors cursor-pointer"
            >
              <Square className="w-3.5 h-3.5 fill-current" />
              <span>Send KILL Packet</span>
            </button>
          )}
        </div>
      </div>

      {/* Main Split Simulator Area: MiSTer Cabinet on Left, PC Server on Right */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* LEFT PANEL: MiSTer Arcade Cabinet & CRT Screen (7 Cols) */}
        <div className="lg:col-span-7 flex flex-col space-y-4">
          <div className="flex items-center justify-between text-xs font-mono text-neutral-400 px-1">
            <div className="flex items-center gap-2">
              <Tv className="w-4 h-4 text-emerald-400" />
              <span className="text-neutral-200 font-semibold">MiSTer Client Screen</span>
              <span>·</span>
              <span>IP: 192.168.1.50</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="text-neutral-400">Core:</span>
              <span className="text-emerald-400 font-medium">
                {misterState === 'STREAMING_ACTIVE' ? 'groovy.rbf (Active)' : 'menu.rbf'}
              </span>
            </div>
          </div>

          {/* CRT Monitor Frame */}
          <div className="relative bg-neutral-900 border-4 border-neutral-800 rounded-2xl p-3 shadow-2xl overflow-hidden aspect-[4/3] flex flex-col">
            
            {/* The Screen Display */}
            <div className="relative flex-1 bg-black rounded-lg overflow-hidden flex flex-col border border-neutral-950">
              
              {/* Optional Scanline Overlay */}
              {scanlinesEnabled && <div className="absolute inset-0 crt-overlay z-20 pointer-events-none" />}

              {/* State 1: MENU_IDLE (MiSTer Arcade Selection Menu) */}
              {misterState === 'MENU_IDLE' && (
                <div className="flex-1 flex flex-col p-4 sm:p-6 text-neutral-200 font-mono text-xs select-none">
                  {/* Top Bar on MiSTer Screen */}
                  <div className="flex items-center justify-between border-b border-neutral-800 pb-2 mb-3">
                    <div className="flex items-center gap-2">
                      <span className="w-2 h-2 rounded-full bg-amber-400 animate-pulse" />
                      <span className="font-bold text-amber-400 crt-bloom tracking-wider">PHANTOM ARCADE v1.2</span>
                    </div>
                    <span className="text-neutral-400">15.7kHz · 240p</span>
                  </div>

                  {/* System Filter Tabs inside MiSTer Menu */}
                  <div className="flex items-center gap-1.5 mb-3 overflow-x-auto pb-1 text-[11px]">
                    {['all', 'ps2', 'gamecube', 'wii', 'naomi', 'model2'].map(sys => (
                      <button
                        key={sys}
                        onClick={() => setActiveSystemFilter(sys)}
                        className={`px-2 py-0.5 rounded uppercase tracking-wider transition-colors cursor-pointer ${
                          activeSystemFilter === sys
                            ? 'bg-amber-400 text-neutral-950 font-bold'
                            : 'bg-neutral-900 text-neutral-400 hover:text-neutral-200'
                        }`}
                      >
                        {sys}
                      </button>
                    ))}
                  </div>

                  {/* Games List in MiSTer Menu Style */}
                  <div className="flex-1 overflow-y-auto space-y-1 pr-1 custom-scrollbar">
                    {filteredGames.map((game, idx) => {
                      const isSelected = selectedGame.id === game.id;
                      return (
                        <div
                          key={game.id}
                          onClick={() => setSelectedGame(game)}
                          onDoubleClick={() => handleLaunch(game)}
                          className={`flex items-center justify-between px-3 py-2 rounded cursor-pointer transition-colors ${
                            isSelected
                              ? 'bg-amber-500/20 border border-amber-400/40 text-amber-300 font-bold shadow-sm'
                              : 'hover:bg-neutral-900 text-neutral-300 border border-transparent'
                          }`}
                        >
                          <div className="flex items-center gap-2.5 truncate">
                            <span className={isSelected ? 'text-amber-400' : 'text-neutral-600'}>
                              {isSelected ? '▶' : ' '}
                            </span>
                            <span className="truncate">{game.title}</span>
                          </div>
                          <div className="flex items-center gap-2 shrink-0 text-[10px] text-neutral-400">
                            <span className="uppercase text-neutral-400">{game.system}</span>
                            <span>·</span>
                            <span>{game.resolution}</span>
                          </div>
                        </div>
                      );
                    })}
                  </div>

                  {/* Bottom Info & Launch Button inside CRT */}
                  <div className="mt-3 pt-3 border-t border-neutral-800 flex items-center justify-between text-[11px]">
                    <div className="text-neutral-400 truncate max-w-[280px]">
                      Target: <span className="text-neutral-200">{selectedGame.systemName}</span>
                    </div>

                    <button
                      onClick={() => handleLaunch(selectedGame)}
                      className="px-3 py-1 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded flex items-center gap-1.5 transition-colors cursor-pointer"
                    >
                      <Play className="w-3 h-3 fill-current" />
                      <span>START (P1)</span>
                    </button>
                  </div>
                </div>
              )}

              {/* State 2: SENDING_UDP / LOADING_CORE */}
              {(misterState === 'SENDING_UDP' || misterState === 'LOADING_CORE') && (
                <div className="flex-1 flex flex-col items-center justify-center p-6 text-center text-neutral-200 font-mono">
                  <div className="w-12 h-12 rounded-full border-2 border-amber-400 border-t-transparent animate-spin mb-4" />
                  <div className="text-sm font-bold text-amber-400 crt-bloom mb-1">
                    {misterState === 'SENDING_UDP' ? 'TRANSMITTING UDP LAUNCH PACKET...' : 'SWAPPING FPGA CORE TO groovy.rbf...'}
                  </div>
                  <div className="text-xs text-neutral-400 max-w-sm">
                    {misterState === 'SENDING_UDP' 
                      ? `Dispatching "LAUNCH:${selectedGame.id}" to PC :2154`
                      : 'DE10-Nano switching into low-latency video listen mode'
                    }
                  </div>
                </div>
              )}

              {/* State 3: STREAMING_ACTIVE (Simulated Game Running on CRT!) */}
              {misterState === 'STREAMING_ACTIVE' && (
                <div className="flex-1 flex flex-col relative overflow-hidden bg-neutral-950">
                  {/* Backdrop Scene reflecting the selected title */}
                  <div className={`absolute inset-0 bg-gradient-to-br ${selectedGame.bannerColor} opacity-90 flex flex-col items-center justify-center p-6 text-center select-none`}>
                    
                    {/* Game Visual Title Banner */}
                    <div className="space-y-2 z-10">
                      <div className="inline-block px-3 py-1 bg-black/60 border border-white/10 rounded text-[11px] font-mono text-amber-300 uppercase tracking-widest backdrop-blur-sm">
                        {selectedGame.systemName} · 15kHz CRT DIRECT
                      </div>
                      <h3 className="text-2xl sm:text-3xl font-extrabold text-white tracking-tight drop-shadow-lg crt-bloom">
                        {selectedGame.title}
                      </h3>
                      <p className="text-xs text-neutral-200/80 max-w-md mx-auto line-clamp-2">
                        {selectedGame.description}
                      </p>
                    </div>

                    {/* Raster / CRT Timing Badge */}
                    <div className="absolute top-3 left-3 bg-black/70 border border-neutral-700/60 rounded px-2.5 py-1 text-[10px] font-mono text-emerald-400 flex items-center gap-1.5 backdrop-blur-sm">
                      <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                      <span>Groovy_MiSTer Sync: {selectedGame.videoMode}</span>
                    </div>

                    <div className="absolute top-3 right-3 bg-black/70 border border-neutral-700/60 rounded px-2.5 py-1 text-[10px] font-mono text-neutral-300 backdrop-blur-sm">
                      RES: {selectedGame.resolution}
                    </div>

                    {/* Interactive Animated Action indicator */}
                    <div className="mt-8 flex items-center gap-2 text-xs font-mono text-white/90 bg-black/50 px-3 py-1.5 rounded-full border border-white/10 backdrop-blur-sm">
                      <Sparkles className="w-3.5 h-3.5 text-amber-400" />
                      <span>LAN Video Feed Active · ~0.8ms Frame Latency</span>
                    </div>
                  </div>

                  {/* Hold Quit Overlay */}
                  {isHoldingQuit && (
                    <div className="absolute inset-0 bg-black/80 z-30 flex flex-col items-center justify-center p-6 backdrop-blur-xs">
                      <div className="text-xs font-mono text-amber-300 font-bold mb-2">
                        EXITING GAME TO MISTER MENU...
                      </div>
                      <div className="w-48 h-2 bg-neutral-800 rounded-full overflow-hidden border border-neutral-700">
                        <div 
                          className="h-full bg-rose-500 transition-all duration-75"
                          style={{ width: `${holdProgress}%` }}
                        />
                      </div>
                      <div className="text-[10px] font-mono text-neutral-400 mt-2">
                        Hold Start + Coin / Select
                      </div>
                    </div>
                  )}
                </div>
              )}

              {/* State 4: KILLING_PROCESS */}
              {misterState === 'KILLING_PROCESS' && (
                <div className="flex-1 flex flex-col items-center justify-center p-6 text-center text-neutral-200 font-mono">
                  <div className="w-10 h-10 rounded-full border-2 border-rose-500 border-t-transparent animate-spin mb-4" />
                  <div className="text-sm font-bold text-rose-400 crt-bloom mb-1">
                    SENDING KILL PACKET...
                  </div>
                  <div className="text-xs text-neutral-400">
                    Terminating PC emulator process and reloading MiSTer menu.rbf
                  </div>
                </div>
              )}
            </div>

            {/* Bottom Cabinet Bezel Label */}
            <div className="mt-2.5 flex items-center justify-between text-[11px] font-mono text-neutral-500 px-2">
              <span className="tracking-widest">NANAO MS9-29 CRT ARCHITECTURE</span>
              <span>15.75 kHz RGB</span>
            </div>
          </div>

          {/* Arcade Cabinet Physical Control Deck */}
          <div className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-3">
            <div className="flex items-center justify-between">
              <span className="text-xs font-medium text-neutral-300 font-mono">Arcade Stick Controls</span>
              <span className="text-[11px] text-neutral-400 font-mono">Simulated P1 Controls</span>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2 text-xs font-mono">
              <button
                disabled={misterState !== 'MENU_IDLE'}
                onClick={() => {
                  const currentIdx = filteredGames.findIndex(g => g.id === selectedGame.id);
                  const nextIdx = (currentIdx - 1 + filteredGames.length) % filteredGames.length;
                  setSelectedGame(filteredGames[nextIdx]);
                }}
                className="p-2 bg-neutral-950 hover:bg-neutral-800 disabled:opacity-40 border border-neutral-800 rounded-lg flex items-center justify-center gap-1.5 text-neutral-300 transition-colors cursor-pointer"
              >
                <ArrowUp className="w-3.5 h-3.5" />
                <span>Up</span>
              </button>

              <button
                disabled={misterState !== 'MENU_IDLE'}
                onClick={() => {
                  const currentIdx = filteredGames.findIndex(g => g.id === selectedGame.id);
                  const nextIdx = (currentIdx + 1) % filteredGames.length;
                  setSelectedGame(filteredGames[nextIdx]);
                }}
                className="p-2 bg-neutral-950 hover:bg-neutral-800 disabled:opacity-40 border border-neutral-800 rounded-lg flex items-center justify-center gap-1.5 text-neutral-300 transition-colors cursor-pointer"
              >
                <ArrowDown className="w-3.5 h-3.5" />
                <span>Down</span>
              </button>

              <button
                disabled={misterState !== 'MENU_IDLE'}
                onClick={() => handleLaunch(selectedGame)}
                className="p-2 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold disabled:opacity-40 rounded-lg flex items-center justify-center gap-1.5 transition-colors cursor-pointer"
              >
                <Play className="w-3.5 h-3.5 fill-current" />
                <span>P1 Start</span>
              </button>

              {/* The Famous Start+Coin Arcade Stick Exit Combo */}
              <button
                disabled={misterState !== 'STREAMING_ACTIVE'}
                onMouseDown={startHoldQuit}
                onMouseUp={cancelHoldQuit}
                onMouseLeave={cancelHoldQuit}
                onTouchStart={startHoldQuit}
                onTouchEnd={cancelHoldQuit}
                className={`p-2 rounded-lg border flex items-center justify-center gap-1.5 transition-colors select-none cursor-pointer ${
                  misterState === 'STREAMING_ACTIVE'
                    ? 'bg-rose-950/60 border-rose-800/80 text-rose-300 hover:bg-rose-900/60 active:scale-95'
                    : 'bg-neutral-950 text-neutral-600 border-neutral-800 opacity-40 cursor-not-allowed'
                }`}
                title="Hold for 1.2s to send KILL packet and return to menu"
              >
                <Square className="w-3.5 h-3.5" />
                <span className="truncate">Hold to Quit</span>
              </button>
            </div>
          </div>
        </div>

        {/* RIGHT PANEL: PC Daemon Server Runtime (5 Cols) */}
        <div className="lg:col-span-5 flex flex-col space-y-4">
          <div className="flex items-center justify-between text-xs font-mono text-neutral-400 px-1">
            <div className="flex items-center gap-2">
              <Server className="w-4 h-4 text-blue-400" />
              <span className="text-neutral-200 font-semibold">PC Background Daemon</span>
            </div>
            <div className="flex items-center gap-1.5">
              <span className="w-2 h-2 rounded-full bg-emerald-400" />
              <span className="text-emerald-400 font-medium">Listening :2154</span>
            </div>
          </div>

          {/* Active Process Status Card */}
          <div className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-3">
            <div className="flex items-center justify-between border-b border-neutral-800 pb-2">
              <span className="text-xs font-semibold text-white">Active Emulator Subprocess</span>
              <span className="text-[11px] font-mono text-neutral-400">
                {activePid ? `PID: ${activePid}` : 'Status: IDLE'}
              </span>
            </div>

            {misterState === 'STREAMING_ACTIVE' && activePid ? (
              <div className="space-y-2 text-xs font-mono">
                <div className="p-2.5 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-1.5">
                  <div className="flex items-center justify-between text-neutral-400">
                    <span>Process:</span>
                    <span className="text-amber-300 font-medium">{selectedGame.emulatorId}.exe</span>
                  </div>
                  <div className="flex items-center justify-between text-neutral-400">
                    <span>Target ROM:</span>
                    <span className="text-neutral-200 truncate max-w-[200px]">{selectedGame.romName}</span>
                  </div>
                  <div className="flex items-center justify-between text-neutral-400">
                    <span>Video Hook:</span>
                    <span className="text-emerald-400">Groovy_MiSTer D3D9</span>
                  </div>
                  <div className="flex items-center justify-between text-neutral-400">
                    <span>Output Sync:</span>
                    <span className="text-blue-400">{selectedGame.videoMode}</span>
                  </div>
                </div>

                <div className="text-[11px] text-neutral-400 bg-neutral-950/60 p-2 rounded border border-neutral-800/60">
                  <div className="text-neutral-400 font-medium mb-1">Command Line Executed:</div>
                  <div className="text-neutral-300 break-all select-all font-mono text-[10px]">
                    {selectedGame.emulatorId}.exe -batch -fullscreen -elf "{selectedGame.romPath}"
                  </div>
                </div>
              </div>
            ) : (
              <div className="p-6 bg-neutral-950/60 rounded-lg border border-neutral-800/60 text-center space-y-1">
                <div className="text-xs text-neutral-300 font-medium">No emulator currently active</div>
                <div className="text-[11px] text-neutral-400">
                  Select a title on the MiSTer screen or click below to launch.
                </div>
              </div>
            )}

            {/* Quick Actions */}
            <div className="pt-2 flex items-center gap-2">
              <button
                onClick={() => handleLaunch(selectedGame)}
                className="flex-1 py-1.5 px-3 bg-neutral-800 hover:bg-neutral-700 text-neutral-200 text-xs font-medium rounded-lg transition-colors cursor-pointer flex items-center justify-center gap-1.5"
              >
                <Zap className="w-3.5 h-3.5 text-amber-400" />
                <span>Launch {selectedGame.title}</span>
              </button>

              <button
                disabled={!activePid}
                onClick={handleKill}
                className="py-1.5 px-3 bg-neutral-800 hover:bg-rose-950 hover:text-rose-300 text-neutral-400 disabled:opacity-30 text-xs font-medium rounded-lg transition-colors cursor-pointer flex items-center justify-center gap-1.5"
              >
                <Square className="w-3.5 h-3.5" />
                <span>Kill PID</span>
              </button>
            </div>
          </div>

          {/* PC Server Live Log Stream */}
          <div className="flex-1 flex flex-col p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-2 min-h-[220px]">
            <div className="flex items-center justify-between border-b border-neutral-800 pb-2">
              <div className="flex items-center gap-2">
                <TerminalIcon className="w-3.5 h-3.5 text-neutral-400" />
                <span className="text-xs font-semibold text-white font-mono">Daemon stdout / stderr</span>
              </div>
              <button
                onClick={() => setServerLogs([`[${new Date().toTimeString().split(' ')[0]}] [INFO] Log buffer cleared.`])}
                className="text-[10px] text-neutral-400 hover:text-neutral-200 font-mono cursor-pointer"
              >
                Clear
              </button>
            </div>

            <div className="flex-1 bg-neutral-950 p-2.5 rounded-lg border border-neutral-800/80 font-mono text-[11px] text-neutral-300 overflow-y-auto space-y-1 max-h-56 custom-scrollbar">
              {serverLogs.map((log, index) => (
                <div 
                  key={index}
                  className={
                    log.includes('[UDP RECV]') ? 'text-amber-300' :
                    log.includes('[PC PROC]') ? 'text-blue-300' :
                    log.includes('[VIDEO]') ? 'text-emerald-300' :
                    log.includes('[STREAM]') ? 'text-purple-300' :
                    'text-neutral-400'
                  }
                >
                  {log}
                </div>
              ))}
            </div>
          </div>

        </div>

      </div>
    </div>
  );
};
