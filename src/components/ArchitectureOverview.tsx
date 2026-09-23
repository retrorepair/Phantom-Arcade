import React from 'react';
import { Cpu, Server, Tv, ArrowRight, Zap, RefreshCw, Layers, ShieldCheck, PlayCircle, HardDrive } from 'lucide-react';

export const ArchitectureOverview: React.FC = () => {
  return (
    <div className="space-y-10 max-w-6xl mx-auto py-6">
      {/* Title & Core Thesis */}
      <div className="border-b border-neutral-800 pb-6">
        <div className="flex items-center gap-2 text-xs text-amber-400 font-mono tracking-wider uppercase mb-2">
          <span>Architecture Blueprint</span>
          <span>·</span>
          <span>Zero FPGA Logic Modification Required</span>
        </div>
        <h1 className="text-3xl font-bold tracking-tight text-white mb-3">
          The Phantom Arcade Architecture
        </h1>
        <p className="text-neutral-400 text-base max-w-3xl leading-relaxed">
          How a lightweight Linux ARM daemon on the MiSTer FPGA pairs with a headless PC server over raw UDP,
          streaming demanding 3D systems (PS2, GameCube, Wii, Sega Model 2/3, Naomi) directly into the{' '}
          <code className="text-amber-300 font-mono text-xs px-1.5 py-0.5 bg-neutral-900 border border-neutral-800 rounded">groovy.rbf</code>{' '}
          core for pixel-perfect 15kHz CRT display.
        </p>
      </div>

      {/* 3-Stage Visual Pipeline Diagram */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 relative">
        {/* Node 1: MiSTer Client */}
        <div className="p-5 bg-neutral-900/80 border border-neutral-800 rounded-xl relative overflow-hidden group hover:border-neutral-700 transition-colors">
          <div className="absolute top-0 right-0 w-28 h-28 bg-emerald-500/5 rounded-bl-full pointer-events-none" />
          <div className="flex items-center justify-between mb-4">
            <div className="w-9 h-9 rounded-lg bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center text-emerald-400">
              <Cpu className="w-5 h-5" />
            </div>
            <span className="text-xs font-mono text-emerald-400">Client Node</span>
          </div>

          <h3 className="text-base font-semibold text-white mb-1">MiSTer FPGA (DE10-Nano)</h3>
          <p className="text-xs text-neutral-400 mb-4 leading-relaxed">
            Main Linux running on ARM Cortex-A9 side handles UI, network packets, and core swaps.
          </p>

          <div className="space-y-2 text-xs font-mono text-neutral-300 bg-neutral-950 p-3 rounded-lg border border-neutral-800/80">
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-emerald-400">1.</span>
              <span>Reads remote games catalog via HTTP</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-emerald-400">2.</span>
              <span>Dispatches sub-ms UDP launch packet</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-emerald-400">3.</span>
              <span>Boots <code className="text-emerald-300">groovy.rbf</code> core</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-emerald-400">4.</span>
              <span>Outputs 15kHz RGB to arcade CRT</span>
            </div>
          </div>
        </div>

        {/* Node 2: Network Link */}
        <div className="p-5 bg-neutral-900/80 border border-neutral-800 rounded-xl relative overflow-hidden group hover:border-neutral-700 transition-colors flex flex-col justify-between">
          <div className="absolute top-0 right-0 w-28 h-28 bg-amber-500/5 rounded-bl-full pointer-events-none" />
          <div>
            <div className="flex items-center justify-between mb-4">
              <div className="w-9 h-9 rounded-lg bg-amber-500/10 border border-amber-500/20 flex items-center justify-center text-amber-400">
                <Zap className="w-5 h-5" />
              </div>
              <span className="text-xs font-mono text-amber-400">Fast Pipeline</span>
            </div>

            <h3 className="text-base font-semibold text-white mb-1">Local Network Protocol</h3>
            <p className="text-xs text-neutral-400 mb-4 leading-relaxed">
              Ultra-low latency LAN/direct Ethernet connection with decoupled command and video channels.
            </p>

            <div className="space-y-2 text-xs font-mono text-neutral-300 bg-neutral-950 p-3 rounded-lg border border-neutral-800/80">
              <div className="flex items-center justify-between text-neutral-400">
                <span>Command Bus:</span>
                <span className="text-amber-300">UDP :2154 (&lt;1ms)</span>
              </div>
              <div className="flex items-center justify-between text-neutral-400">
                <span>Catalog Sync:</span>
                <span className="text-amber-300">HTTP :8088 JSON</span>
              </div>
              <div className="flex items-center justify-between text-neutral-400">
                <span>Video Stream:</span>
                <span className="text-amber-300">Groovy Protocol</span>
              </div>
              <div className="flex items-center justify-between text-neutral-400">
                <span>Input Path:</span>
                <span className="text-amber-300">USB Arcade Stick</span>
              </div>
            </div>
          </div>

          <div className="mt-4 pt-3 border-t border-neutral-800 flex items-center justify-center gap-2 text-xs text-neutral-400 font-mono">
            <span>MiSTer</span>
            <ArrowRight className="w-3.5 h-3.5 text-amber-400" />
            <span>Headless PC</span>
          </div>
        </div>

        {/* Node 3: Headless PC Server */}
        <div className="p-5 bg-neutral-900/80 border border-neutral-800 rounded-xl relative overflow-hidden group hover:border-neutral-700 transition-colors">
          <div className="absolute top-0 right-0 w-28 h-28 bg-blue-500/5 rounded-bl-full pointer-events-none" />
          <div className="flex items-center justify-between mb-4">
            <div className="w-9 h-9 rounded-lg bg-blue-500/10 border border-blue-500/20 flex items-center justify-center text-blue-400">
              <Server className="w-5 h-5" />
            </div>
            <span className="text-xs font-mono text-blue-400">Server Node</span>
          </div>

          <h3 className="text-base font-semibold text-white mb-1">Headless PC Daemon</h3>
          <p className="text-xs text-neutral-400 mb-4 leading-relaxed">
            Silent background service running on Windows/Linux host hidden under desk or closet.
          </p>

          <div className="space-y-2 text-xs font-mono text-neutral-300 bg-neutral-950 p-3 rounded-lg border border-neutral-800/80">
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-blue-400">1.</span>
              <span>Listens for UDP packets from MiSTer IP</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-blue-400">2.</span>
              <span>Executes emulator silently in batch mode</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-blue-400">3.</span>
              <span>Streams raw frames via Groovy hook</span>
            </div>
            <div className="flex items-center gap-1.5 text-neutral-400">
              <span className="text-blue-400">4.</span>
              <span>Terminates emulator on KILL packet</span>
            </div>
          </div>
        </div>
      </div>

      {/* Deep Dive Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Card 1: Why No FPGA Modifications Needed */}
        <div className="p-6 bg-neutral-900/50 border border-neutral-800 rounded-xl">
          <div className="flex items-center gap-3 mb-3">
            <div className="p-2 bg-neutral-800 rounded-lg text-amber-400">
              <Layers className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-semibold text-white">Why No FPGA Core Edits Are Needed</h3>
          </div>
          <p className="text-neutral-400 text-sm leading-relaxed mb-4">
            The DE10-Nano is an SoC containing both an <strong>FPGA fabric</strong> and a <strong>Dual-Core ARM Cortex-A9</strong> running Main Linux.
            Instead of modifying Verilog/VHDL code:
          </p>
          <ul className="space-y-2.5 text-sm text-neutral-300">
            <li className="flex items-start gap-2">
              <span className="text-amber-400 font-bold">·</span>
              <span>
                <strong>Calamity's Groovy_MiSTer core</strong> (<code className="text-amber-300 font-mono text-xs">groovy.rbf</code>) already exists as a general-purpose frame sink capable of receiving raw pixel streams over Ethernet/USB and pushing them to the IO board DAC.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="text-amber-400 font-bold">·</span>
              <span>
                The standard MiSTer control device (<code className="text-amber-300 font-mono text-xs">/dev/MiSTer_cmd</code>) accepts commands from user scripts to switch between cores on the fly without rebooting.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="text-amber-400 font-bold">·</span>
              <span>
                All orchestrations happen purely in userspace Linux on the MiSTer and a standard Python daemon on the PC.
              </span>
            </li>
          </ul>
        </div>

        {/* Card 2: The Hotkey Kill Switch Loop */}
        <div className="p-6 bg-neutral-900/50 border border-neutral-800 rounded-xl">
          <div className="flex items-center gap-3 mb-3">
            <div className="p-2 bg-neutral-800 rounded-lg text-emerald-400">
              <ShieldCheck className="w-5 h-5" />
            </div>
            <h3 className="text-lg font-semibold text-white">Clean Process State Management</h3>
          </div>
          <p className="text-neutral-400 text-sm leading-relaxed mb-4">
            Arcade cabinets don't have keyboards or mice. When the player wants to change games, the system must cleanly reset:
          </p>
          <ul className="space-y-2.5 text-sm text-neutral-300">
            <li className="flex items-start gap-2">
              <span className="text-emerald-400 font-bold">·</span>
              <span>
                <strong>Hotkey Combo:</strong> Holding <code className="text-emerald-300 font-mono text-xs">P1 Start + Coin</code> (or Select) for 1.5 seconds triggers the MiSTer hotkey daemon.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="text-emerald-400 font-bold">·</span>
              <span>
                <strong>Instant Kill Packet:</strong> The client emits a <code className="text-emerald-300 font-mono text-xs">KILL</code> UDP packet to the PC server.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="text-emerald-400 font-bold">·</span>
              <span>
                <strong>Process Termination:</strong> The PC daemon terminates PCSX2/Dolphin subprocess with clean PID tracking, clearing GPU memory for the next title.
              </span>
            </li>
            <li className="flex items-start gap-2">
              <span className="text-emerald-400 font-bold">·</span>
              <span>
                <strong>Menu Restoration:</strong> MiSTer ARM automatically writes <code className="text-emerald-300 font-mono text-xs">load_core /media/fat/menu.rbf</code> back to the command pipe, returning the player to the cabinet menu instantly.
              </span>
            </li>
          </ul>
        </div>
      </div>

      {/* Comparison Table */}
      <div className="p-6 bg-neutral-900/40 border border-neutral-800 rounded-xl">
        <h3 className="text-lg font-semibold text-white mb-2">Native MiSTer FPGA vs. Phantom Hybrid Architecture</h3>
        <p className="text-xs text-neutral-400 mb-6">
          Comparing capabilities across processing power, supported systems, and CRT video authenticity.
        </p>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-sm">
            <thead>
              <tr className="border-b border-neutral-800 text-xs font-mono text-neutral-400">
                <th className="pb-3 font-medium">Dimension</th>
                <th className="pb-3 font-medium text-neutral-400">Native MiSTer Alone</th>
                <th className="pb-3 font-medium text-amber-400">Phantom Arcade (MiSTer + Headless PC)</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-neutral-800/60 font-mono text-xs">
              <tr>
                <td className="py-3 text-neutral-300 font-sans font-medium">System Ceiling</td>
                <td className="py-3 text-neutral-400">PS1, Saturn, N64, NeoGeo, CPS1/2/3</td>
                <td className="py-3 text-amber-300 font-medium">PS2, GameCube, Wii, Naomi 2, Model 2/3, RPCS3</td>
              </tr>
              <tr>
                <td className="py-3 text-neutral-300 font-sans font-medium">Video Output</td>
                <td className="py-3 text-neutral-400">Native 15kHz analog RGB via IO Board</td>
                <td className="py-3 text-emerald-400">Identical 15kHz native CRT RGB via groovy.rbf</td>
              </tr>
              <tr>
                <td className="py-3 text-neutral-300 font-sans font-medium">Arcade Controls</td>
                <td className="py-3 text-neutral-400">Direct USB or SNAC to FPGA</td>
                <td className="py-3 text-emerald-400">Direct USB to MiSTer with input pass-through</td>
              </tr>
              <tr>
                <td className="py-3 text-neutral-300 font-sans font-medium">User Perception</td>
                <td className="py-3 text-neutral-400">Appears 100% native</td>
                <td className="py-3 text-amber-300">Indistinguishable from native console on CRT</td>
              </tr>
              <tr>
                <td className="py-3 text-neutral-300 font-sans font-medium">Storage Requirement</td>
                <td className="py-3 text-neutral-400">Large MicroSD cards on MiSTer</td>
                <td className="py-3 text-emerald-400">PC high-capacity NVMe/SATA library hosted over LAN</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
