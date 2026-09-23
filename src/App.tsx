import React, { useState } from 'react';
import { GameItem, EmulatorProfile, MiSTerState, PacketLog } from './types';
import { INITIAL_GAMES, DEFAULT_EMULATORS } from './data/mockCatalog';
import { TopBar } from './components/TopBar';
import { ArcadeSimulator } from './components/ArcadeSimulator';
import { WindowsSetupApp } from './components/WindowsSetupApp';
import { LibraryManager } from './components/LibraryManager';
import { PacketInspector } from './components/PacketInspector';
import { ModelineCalculator } from './components/ModelineCalculator';
import { ScriptExporter } from './components/ScriptExporter';
import { ArchitectureOverview } from './components/ArchitectureOverview';

export default function App() {
  const [activeTab, setActiveTab] = useState<string>('simulator');
  const [games, setGames] = useState<GameItem[]>(INITIAL_GAMES);
  const [emulators, setEmulators] = useState<EmulatorProfile[]>(DEFAULT_EMULATORS);
  const [selectedGame, setSelectedGame] = useState<GameItem>(INITIAL_GAMES[0]);
  const [misterState, setMisterState] = useState<MiSTerState>('MENU_IDLE');
  
  // Real-time packet bus logs
  const [packetLogs, setPacketLogs] = useState<PacketLog[]>([
    {
      id: 'pkt_init_01',
      timestamp: '21:04:15.102',
      type: 'PING',
      direction: 'MISTER_TO_PC',
      protocol: 'UDP',
      port: 2154,
      payload: 'PING',
      latencyMs: 0.45,
      status: 'DELIVERED'
    },
    {
      id: 'pkt_init_02',
      timestamp: '21:04:15.103',
      type: 'ACK',
      direction: 'PC_TO_MISTER',
      protocol: 'UDP',
      port: 2154,
      payload: 'PONG:PHANTOM_ONLINE',
      latencyMs: 0.38,
      status: 'PROCESSED'
    }
  ]);

  const handleSendPacket = (type: 'LAUNCH' | 'KILL' | 'PING', payload: string) => {
    const now = new Date();
    const timeStr = `${now.toTimeString().split(' ')[0]}.${now.getMilliseconds().toString().padStart(3, '0')}`;
    const id = `pkt_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
    const latency = parseFloat((0.35 + Math.random() * 0.65).toFixed(2));

    const outgoingPacket: PacketLog = {
      id,
      timestamp: timeStr,
      type,
      direction: 'MISTER_TO_PC',
      protocol: 'UDP',
      port: 2154,
      payload: type === 'LAUNCH' ? `LAUNCH:${payload}` : payload,
      latencyMs: latency,
      status: 'DELIVERED'
    };

    setPacketLogs(prev => [outgoingPacket, ...prev.slice(0, 49)]);

    // Simulate PC ACK response packet after 20ms
    setTimeout(() => {
      const ackTime = new Date();
      const ackTimeStr = `${ackTime.toTimeString().split(' ')[0]}.${ackTime.getMilliseconds().toString().padStart(3, '0')}`;
      const ackPacket: PacketLog = {
        id: `pkt_ack_${Date.now()}`,
        timestamp: ackTimeStr,
        type: 'ACK',
        direction: 'PC_TO_MISTER',
        protocol: 'UDP',
        port: 2154,
        payload: type === 'LAUNCH' ? `ACK:LAUNCH:${payload}:SUCCESS` : `ACK:${type}:SUCCESS`,
        latencyMs: parseFloat((0.28 + Math.random() * 0.4).toFixed(2)),
        status: 'PROCESSED'
      };
      setPacketLogs(prev => [ackPacket, ...prev.slice(0, 49)]);
    }, 25);
  };

  const handleClearLogs = () => {
    setPacketLogs([]);
  };

  const handleQuickSimulate = () => {
    setActiveTab('simulator');
  };

  const handleOpenExports = () => {
    setActiveTab('exports');
  };

  const handleLaunchGameFromLibrary = (game: GameItem) => {
    setSelectedGame(game);
    setActiveTab('simulator');
    setMisterState('SENDING_UDP');
    handleSendPacket('LAUNCH', game.id);

    setTimeout(() => {
      setMisterState('LOADING_CORE');
      setTimeout(() => {
        setMisterState('STREAMING_ACTIVE');
      }, 700);
    }, 400);
  };

  return (
    <div className="min-h-screen bg-neutral-950 text-neutral-100 flex flex-col font-sans">
      {/* Top Bar following Top Bar Contract */}
      <TopBar
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        serverActive={true}
        onQuickSimulate={handleQuickSimulate}
        onOpenExports={handleOpenExports}
      />

      {/* Main Content Area */}
      <main className="flex-1 px-4 sm:px-6 lg:px-8 py-6">
        {activeTab === 'simulator' && (
          <ArcadeSimulator
            games={games}
            selectedGame={selectedGame}
            setSelectedGame={setSelectedGame}
            misterState={misterState}
            setMisterState={setMisterState}
            onSendPacket={handleSendPacket}
            serverActive={true}
          />
        )}

        {activeTab === 'windows' && (
          <WindowsSetupApp />
        )}

        {activeTab === 'library' && (
          <LibraryManager
            games={games}
            setGames={setGames}
            emulators={emulators}
            setEmulators={setEmulators}
            onLaunchGame={handleLaunchGameFromLibrary}
          />
        )}

        {activeTab === 'network' && (
          <PacketInspector
            packetLogs={packetLogs}
            onClearLogs={handleClearLogs}
            onInjectPacket={handleSendPacket}
          />
        )}

        {activeTab === 'modelines' && (
          <ModelineCalculator />
        )}

        {activeTab === 'exports' && (
          <ScriptExporter />
        )}

        {activeTab === 'architecture' && (
          <ArchitectureOverview />
        )}
      </main>

      {/* Clean Unboxed Footer without Ornamental Fake Telemetry */}
      <footer className="border-t border-neutral-800/80 px-6 py-5 text-xs text-neutral-400 bg-neutral-950">
        <div className="max-w-7xl mx-auto flex flex-col sm:flex-row items-center justify-between gap-3">
          <div className="flex items-center gap-2">
            <span className="font-semibold text-neutral-300">Phantom Arcade Bridge</span>
            <span aria-hidden="true">·</span>
            <span>Groovy_MiSTer FPGA + Headless PC Architecture</span>
          </div>

          <div className="flex items-center gap-3 font-mono text-[11px] text-neutral-400">
            <span>UDP :2154</span>
            <span aria-hidden="true">·</span>
            <span>HTTP :8088</span>
            <span aria-hidden="true">·</span>
            <span>groovy.rbf 15kHz CRT</span>
          </div>
        </div>
      </footer>
    </div>
  );
}
