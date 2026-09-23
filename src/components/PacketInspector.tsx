import React, { useState } from 'react';
import { PacketLog } from '../types';
import { Activity, Send, Filter, Clock, CheckCircle2, AlertCircle, ArrowUpRight, ArrowDownLeft, Terminal, Trash2 } from 'lucide-react';

interface PacketInspectorProps {
  packetLogs: PacketLog[];
  onClearLogs: () => void;
  onInjectPacket: (type: 'LAUNCH' | 'KILL' | 'PING', payload: string) => void;
}

export const PacketInspector: React.FC<PacketInspectorProps> = ({
  packetLogs,
  onClearLogs,
  onInjectPacket
}) => {
  const [filterType, setFilterType] = useState<string>('ALL');
  const [customCommand, setCustomCommand] = useState('LAUNCH:ps2_arcana_heart');
  const [selectedPacket, setSelectedPacket] = useState<PacketLog | null>(null);

  const filteredLogs = filterType === 'ALL'
    ? packetLogs
    : packetLogs.filter(p => p.type === filterType);

  const handleSendCustom = (e: React.FormEvent) => {
    e.preventDefault();
    if (!customCommand.trim()) return;

    if (customCommand.startsWith('LAUNCH:')) {
      onInjectPacket('LAUNCH', customCommand.substring(7));
    } else if (customCommand === 'KILL' || customCommand.startsWith('KILL:')) {
      onInjectPacket('KILL', 'MANUAL_INJECT');
    } else if (customCommand === 'PING') {
      onInjectPacket('PING', '');
    } else {
      onInjectPacket('LAUNCH', customCommand);
    }
  };

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Title & Stats */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2">
            <span>Network Packet Bus & Telemetry</span>
            <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-neutral-400">
              UDP Port 2154
            </span>
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Real-time packet monitor capturing sub-millisecond command handshakes between MiSTer Linux ARM and the PC daemon.
          </p>
        </div>

        {/* Filter buttons */}
        <div className="flex items-center gap-1.5 p-1 bg-neutral-900 border border-neutral-800 rounded-lg text-xs font-mono">
          {['ALL', 'LAUNCH', 'KILL', 'PING', 'ACK'].map((f) => (
            <button
              key={f}
              onClick={() => setFilterType(f)}
              className={`px-2.5 py-1 rounded transition-colors cursor-pointer ${
                filterType === f
                  ? 'bg-neutral-800 text-amber-400 font-semibold shadow-xs'
                  : 'text-neutral-400 hover:text-neutral-200'
              }`}
            >
              {f}
            </button>
          ))}
        </div>
      </div>

      {/* Packet Injector Bar */}
      <form onSubmit={handleSendCustom} className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-2">
        <div className="flex items-center justify-between">
          <span className="text-xs font-semibold text-neutral-300 font-mono flex items-center gap-2">
            <Activity className="w-3.5 h-3.5 text-amber-400" />
            <span>Interactive UDP Packet Injector</span>
          </span>
          <span className="text-[11px] text-neutral-400 font-mono">Simulates network payload from MiSTer IP</span>
        </div>

        <div className="flex items-center gap-2">
          <input
            type="text"
            value={customCommand}
            onChange={(e) => setCustomCommand(e.target.value)}
            placeholder="e.g. LAUNCH:ps2_arcana_heart, KILL, PING"
            className="flex-1 bg-neutral-950 border border-neutral-800 rounded-lg px-3 py-2 text-xs text-amber-300 font-mono focus:outline-none focus:border-amber-400"
          />

          <button
            type="submit"
            className="px-4 py-2 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded-lg text-xs flex items-center gap-1.5 transition-colors cursor-pointer shrink-0"
          >
            <Send className="w-3.5 h-3.5" />
            <span>Dispatch Packet</span>
          </button>
        </div>

        <div className="flex items-center gap-2 pt-1 text-[11px] text-neutral-400 font-mono">
          <span>Quick Injectors:</span>
          <button
            type="button"
            onClick={() => { setCustomCommand('LAUNCH:ps2_arcana_heart'); onInjectPacket('LAUNCH', 'ps2_arcana_heart'); }}
            className="text-amber-400/80 hover:text-amber-300 underline cursor-pointer"
          >
            LAUNCH:ps2_arcana_heart
          </button>
          <span>·</span>
          <button
            type="button"
            onClick={() => { setCustomCommand('KILL'); onInjectPacket('KILL', 'USER_HOTKEY'); }}
            className="text-rose-400/80 hover:text-rose-300 underline cursor-pointer"
          >
            KILL
          </button>
          <span>·</span>
          <button
            type="button"
            onClick={() => { setCustomCommand('PING'); onInjectPacket('PING', ''); }}
            className="text-blue-400/80 hover:text-blue-300 underline cursor-pointer"
          >
            PING
          </button>
        </div>
      </form>

      {/* Split View: Packet Stream Table + Detailed Inspection Box */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* Left: Packets Table (8 cols) */}
        <div className="lg:col-span-8 bg-neutral-900/60 border border-neutral-800 rounded-xl overflow-hidden flex flex-col">
          <div className="flex items-center justify-between px-4 py-3 border-b border-neutral-800 bg-neutral-900/80 text-xs font-mono">
            <span className="text-neutral-300 font-semibold">Captured Network Frames ({filteredLogs.length})</span>
            <button
              onClick={onClearLogs}
              className="text-neutral-500 hover:text-neutral-300 flex items-center gap-1 text-[11px] cursor-pointer"
            >
              <Trash2 className="w-3 h-3" />
              <span>Clear History</span>
            </button>
          </div>

          <div className="overflow-x-auto max-h-[500px] overflow-y-auto custom-scrollbar">
            <table className="w-full text-left text-xs font-mono">
              <thead className="sticky top-0 bg-neutral-950 border-b border-neutral-800 text-neutral-400">
                <tr>
                  <th className="py-2.5 px-3">Time</th>
                  <th className="py-2.5 px-3">Dir</th>
                  <th className="py-2.5 px-3">Type</th>
                  <th className="py-2.5 px-3">Payload</th>
                  <th className="py-2.5 px-3 text-right">Latency</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-neutral-800/60">
                {filteredLogs.length === 0 ? (
                  <tr>
                    <td colSpan={5} className="py-12 text-center text-neutral-500 font-sans">
                      No network packets captured yet. Use the simulator or injector above.
                    </td>
                  </tr>
                ) : (
                  filteredLogs.map((p) => {
                    const isSelected = selectedPacket?.id === p.id;
                    return (
                      <tr
                        key={p.id}
                        onClick={() => setSelectedPacket(p)}
                        className={`cursor-pointer transition-colors ${
                          isSelected
                            ? 'bg-amber-500/15 text-amber-200'
                            : 'hover:bg-neutral-800/40 text-neutral-300'
                        }`}
                      >
                        <td className="py-2.5 px-3 text-neutral-400 tabular-nums">
                          {p.timestamp}
                        </td>

                        <td className="py-2.5 px-3">
                          {p.direction === 'MISTER_TO_PC' ? (
                            <span className="text-emerald-400 flex items-center gap-1">
                              <ArrowUpRight className="w-3 h-3" />
                              <span>M→PC</span>
                            </span>
                          ) : (
                            <span className="text-blue-400 flex items-center gap-1">
                              <ArrowDownLeft className="w-3 h-3" />
                              <span>PC→M</span>
                            </span>
                          )}
                        </td>

                        <td className="py-2.5 px-3">
                          <span className={`px-1.5 py-0.5 rounded text-[10px] font-bold ${
                            p.type === 'LAUNCH' ? 'bg-amber-400/20 text-amber-300' :
                            p.type === 'KILL' ? 'bg-rose-400/20 text-rose-300' :
                            p.type === 'ACK' ? 'bg-emerald-400/20 text-emerald-300' :
                            'bg-blue-400/20 text-blue-300'
                          }`}>
                            {p.type}
                          </span>
                        </td>

                        <td className="py-2.5 px-3 text-neutral-200 truncate max-w-[240px]">
                          {p.payload}
                        </td>

                        <td className="py-2.5 px-3 text-right text-emerald-400 tabular-nums">
                          {p.latencyMs.toFixed(2)} ms
                        </td>
                      </tr>
                    );
                  })
                )}
              </tbody>
            </table>
          </div>
        </div>

        {/* Right: Selected Packet Inspector (4 cols) */}
        <div className="lg:col-span-4 bg-neutral-900 border border-neutral-800 rounded-xl p-4 space-y-4">
          <div className="border-b border-neutral-800 pb-2">
            <h3 className="text-xs font-semibold text-white font-mono">Packet Dissector</h3>
            <p className="text-[11px] text-neutral-400">Click any frame on the left to inspect raw bytes.</p>
          </div>

          {selectedPacket ? (
            <div className="space-y-3 font-mono text-xs">
              <div className="bg-neutral-950 p-3 rounded-lg border border-neutral-800/80 space-y-1.5">
                <div className="flex justify-between text-neutral-400">
                  <span>Packet ID:</span>
                  <span className="text-neutral-200">{selectedPacket.id}</span>
                </div>
                <div className="flex justify-between text-neutral-400">
                  <span>Transport:</span>
                  <span className="text-amber-400">{selectedPacket.protocol} : {selectedPacket.port}</span>
                </div>
                <div className="flex justify-between text-neutral-400">
                  <span>Source / Dest:</span>
                  <span className="text-neutral-200">
                    {selectedPacket.direction === 'MISTER_TO_PC' ? '192.168.1.50 -> PC' : 'PC -> 192.168.1.50'}
                  </span>
                </div>
                <div className="flex justify-between text-neutral-400">
                  <span>Transit Time:</span>
                  <span className="text-emerald-400 tabular-nums">{selectedPacket.latencyMs.toFixed(2)} ms</span>
                </div>
              </div>

              <div>
                <div className="text-[11px] text-neutral-400 mb-1">Payload Content (UTF-8):</div>
                <div className="bg-neutral-950 p-2.5 rounded border border-neutral-800 text-amber-300 break-all">
                  {selectedPacket.payload}
                </div>
              </div>

              <div>
                <div className="text-[11px] text-neutral-400 mb-1">Simulated Hex Dump:</div>
                <div className="bg-neutral-950 p-2.5 rounded border border-neutral-800 text-[10px] text-neutral-400 font-mono overflow-x-auto">
                  {selectedPacket.payload
                    .split('')
                    .map((c) => c.charCodeAt(0).toString(16).padStart(2, '0').toUpperCase())
                    .join(' ')}
                </div>
              </div>
            </div>
          ) : (
            <div className="p-8 text-center text-xs text-neutral-500 font-mono">
              Select a packet from the table to view transmission metadata and payload breakdown.
            </div>
          )}
        </div>

      </div>
    </div>
  );
};
