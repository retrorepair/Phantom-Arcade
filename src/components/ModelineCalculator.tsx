import React, { useState } from 'react';
import { ModelinePreset } from '../types';
import { MODELINE_PRESETS } from '../data/mockCatalog';
import { Tv, Sliders, AlertTriangle, CheckCircle2, Copy, Check } from 'lucide-react';

export const ModelineCalculator: React.FC = () => {
  const [selectedPreset, setSelectedPreset] = useState<ModelinePreset>(MODELINE_PRESETS[0]);
  const [hActive, setHActive] = useState<number>(640);
  const [vActive, setVActive] = useState<number>(224);
  const [refreshRate, setRefreshRate] = useState<number>(59.94);
  const [isInterlaced, setIsInterlaced] = useState<boolean>(false);
  const [copied, setCopied] = useState<boolean>(false);

  // Apply preset
  const handleSelectPreset = (preset: ModelinePreset) => {
    setSelectedPreset(preset);
    setHActive(preset.hActive);
    setVActive(preset.vActive);
    setRefreshRate(preset.refreshRate);
    setIsInterlaced(preset.interlaced);
  };

  // Calculation parameters
  const hFront = selectedPreset.hFront;
  const hSync = selectedPreset.hSync;
  const hBack = selectedPreset.hBack;
  const hTotal = hActive + hFront + hSync + hBack;

  const vFront = selectedPreset.vFront;
  const vSync = selectedPreset.vSync;
  const vBack = selectedPreset.vBack;
  const vTotal = vActive + vFront + vSync + vBack;

  // Calculated frequencies
  const vLinesPerFrame = isInterlaced ? vTotal / 2 : vTotal;
  const hFreqKHz = (refreshRate * vTotal) / 1000;
  const pixelClockMHz = (hTotal * hFreqKHz) / 1000;

  // Monitor chassis classification
  const is15kHz = hFreqKHz >= 15.0 && hFreqKHz <= 16.5;
  const is24kHz = hFreqKHz >= 23.5 && hFreqKHz <= 26.0;
  const is31kHz = hFreqKHz >= 30.5 && hFreqKHz <= 33.5;

  const modelineString = `"${hActive}x${vActive}_${Math.round(refreshRate)}${isInterlaced ? 'i' : ''}" ${pixelClockMHz.toFixed(3)} ${hActive} ${hActive + hFront} ${hActive + hFront + hSync} ${hTotal} ${vActive} ${vActive + vFront} ${vActive + vFront + vSync} ${vTotal} ${isInterlaced ? 'interlace ' : ''}-hsync -vsync`;

  const copyModeline = () => {
    navigator.clipboard.writeText(modelineString);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Title */}
      <div className="border-b border-neutral-800 pb-4">
        <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2">
          <span>CRT Modeline & SwitchRes Calculator</span>
          <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-neutral-400">
            Groovy_MiSTer Video Sync
          </span>
        </h2>
        <p className="text-xs text-neutral-400 mt-1">
          Compute accurate analog CRT video timings for 15kHz standard resolution, 24kHz medium resolution, and 31kHz arcade monitors.
        </p>
      </div>

      {/* Presets Grid */}
      <div>
        <div className="text-xs font-mono text-neutral-400 mb-2">Preset Arcade & Console Standards:</div>
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
          {MODELINE_PRESETS.map((preset) => (
            <button
              key={preset.name}
              onClick={() => handleSelectPreset(preset)}
              className={`p-3 rounded-xl border text-left transition-colors cursor-pointer ${
                selectedPreset.name === preset.name
                  ? 'bg-amber-500/10 border-amber-400/50 text-white'
                  : 'bg-neutral-900 border-neutral-800 text-neutral-400 hover:text-neutral-200 hover:border-neutral-700'
              }`}
            >
              <div className="text-xs font-semibold text-white truncate">{preset.name}</div>
              <div className="text-[11px] text-amber-400 font-mono mt-1">
                {preset.hFreq} · {preset.hActive}x{preset.vActive}{preset.interlaced ? 'i' : 'p'}
              </div>
              <div className="text-[10px] text-neutral-500 mt-0.5">{preset.system}</div>
            </button>
          ))}
        </div>
      </div>

      {/* Interactive Controls & Telemetry */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* Sliders & Configuration (7 cols) */}
        <div className="lg:col-span-7 bg-neutral-900 border border-neutral-800 rounded-xl p-5 space-y-4">
          <div className="flex items-center justify-between border-b border-neutral-800 pb-3">
            <span className="text-xs font-semibold text-white font-mono flex items-center gap-2">
              <Sliders className="w-3.5 h-3.5 text-amber-400" />
              <span>Raster Geometry & Timing Parameters</span>
            </span>
            <span className="text-[11px] font-mono text-neutral-400">CRT Deflection Values</span>
          </div>

          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs font-mono">
            <div>
              <label className="block text-neutral-400 mb-1">
                Horizontal Active (Pixels): <span className="text-white font-bold">{hActive}</span>
              </label>
              <input
                type="range"
                min="256"
                max="800"
                step="8"
                value={hActive}
                onChange={(e) => setHActive(Number(e.target.value))}
                className="w-full accent-amber-400"
              />
              <div className="flex justify-between text-[10px] text-neutral-500 mt-0.5">
                <span>256 (NeoGeo)</span>
                <span>496 (Model 2)</span>
                <span>640 (PS2/DC)</span>
              </div>
            </div>

            <div>
              <label className="block text-neutral-400 mb-1">
                Vertical Active (Lines): <span className="text-white font-bold">{vActive}</span>
              </label>
              <input
                type="range"
                min="200"
                max="512"
                step="4"
                value={vActive}
                onChange={(e) => setVActive(Number(e.target.value))}
                className="w-full accent-amber-400"
              />
              <div className="flex justify-between text-[10px] text-neutral-500 mt-0.5">
                <span>224 (CPS2)</span>
                <span>240 (Standard)</span>
                <span>480 (Interlaced)</span>
              </div>
            </div>

            <div>
              <label className="block text-neutral-400 mb-1">
                Vertical Refresh Rate: <span className="text-white font-bold">{refreshRate} Hz</span>
              </label>
              <input
                type="range"
                min="50.0"
                max="60.5"
                step="0.05"
                value={refreshRate}
                onChange={(e) => setRefreshRate(Number(e.target.value))}
                className="w-full accent-amber-400"
              />
              <div className="flex justify-between text-[10px] text-neutral-500 mt-0.5">
                <span>50.0Hz (PAL)</span>
                <span>59.94Hz (NTSC)</span>
                <span>60.0Hz</span>
              </div>
            </div>

            <div className="flex flex-col justify-center">
              <label className="text-neutral-400 mb-1.5">Scanning Mode</label>
              <div className="flex items-center gap-2">
                <button
                  type="button"
                  onClick={() => setIsInterlaced(false)}
                  className={`flex-1 py-1.5 rounded border text-center transition-colors cursor-pointer ${
                    !isInterlaced
                      ? 'bg-amber-400 text-neutral-950 font-bold border-amber-400'
                      : 'bg-neutral-950 text-neutral-400 border-neutral-800'
                  }`}
                >
                  Progressive (240p)
                </button>
                <button
                  type="button"
                  onClick={() => setIsInterlaced(true)}
                  className={`flex-1 py-1.5 rounded border text-center transition-colors cursor-pointer ${
                    isInterlaced
                      ? 'bg-amber-400 text-neutral-950 font-bold border-amber-400'
                      : 'bg-neutral-950 text-neutral-400 border-neutral-800'
                  }`}
                >
                  Interlaced (480i)
                </button>
              </div>
            </div>
          </div>

          {/* Porches & Blanking table */}
          <div className="pt-2">
            <div className="text-[11px] text-neutral-400 font-mono mb-1.5">Blanking & Sync Timings:</div>
            <div className="grid grid-cols-4 gap-2 text-center text-xs font-mono bg-neutral-950 p-2.5 rounded-lg border border-neutral-800">
              <div>
                <div className="text-[10px] text-neutral-500">H-Front</div>
                <div className="text-white font-bold">{hFront} px</div>
              </div>
              <div>
                <div className="text-[10px] text-neutral-500">H-Sync</div>
                <div className="text-white font-bold">{hSync} px</div>
              </div>
              <div>
                <div className="text-[10px] text-neutral-500">V-Front</div>
                <div className="text-white font-bold">{vFront} lines</div>
              </div>
              <div>
                <div className="text-[10px] text-neutral-500">V-Sync</div>
                <div className="text-white font-bold">{vSync} lines</div>
              </div>
            </div>
          </div>
        </div>

        {/* Chassis Telemetry & Safety Monitor (5 cols) */}
        <div className="lg:col-span-5 bg-neutral-900 border border-neutral-800 rounded-xl p-5 space-y-4">
          <div className="flex items-center justify-between border-b border-neutral-800 pb-3">
            <span className="text-xs font-semibold text-white font-mono flex items-center gap-2">
              <Tv className="w-3.5 h-3.5 text-emerald-400" />
              <span>Chassis Deflection Telemetry</span>
            </span>
            <span className="text-[11px] font-mono text-neutral-400">Flyback Load</span>
          </div>

          <div className="space-y-3 font-mono text-xs">
            <div className="p-3 bg-neutral-950 rounded-lg border border-neutral-800/80 space-y-2">
              <div className="flex justify-between items-center text-neutral-400">
                <span>Horizontal Scan Freq:</span>
                <span className="text-lg font-bold text-amber-300 tabular-nums">
                  {hFreqKHz.toFixed(2)} kHz
                </span>
              </div>

              <div className="flex justify-between items-center text-neutral-400">
                <span>Dot / Pixel Clock:</span>
                <span className="text-white font-medium tabular-nums">
                  {pixelClockMHz.toFixed(3)} MHz
                </span>
              </div>

              <div className="flex justify-between items-center text-neutral-400">
                <span>Active Raster Lines:</span>
                <span className="text-white tabular-nums">
                  {vActive} {isInterlaced ? 'lines (Interlaced)' : 'lines (Progressive)'}
                </span>
              </div>
            </div>

            {/* Monitor Chassis Safety Status */}
            <div className={`p-3 rounded-lg border flex items-start gap-2.5 ${
              is15kHz ? 'bg-emerald-950/40 border-emerald-800/60 text-emerald-300' :
              is24kHz ? 'bg-amber-950/40 border-amber-800/60 text-amber-300' :
              is31kHz ? 'bg-blue-950/40 border-blue-800/60 text-blue-300' :
              'bg-rose-950/40 border-rose-800/60 text-rose-300'
            }`}>
              {is15kHz || is24kHz || is31kHz ? (
                <CheckCircle2 className="w-4 h-4 shrink-0 mt-0.5" />
              ) : (
                <AlertTriangle className="w-4 h-4 shrink-0 mt-0.5" />
              )}
              <div className="text-xs">
                <div className="font-bold">
                  {is15kHz ? 'Safe for 15kHz Arcade CRT / PVM / Consumer TV' :
                   is24kHz ? '24kHz Medium Resolution (Sega Model 2 / Model 3)' :
                   is31kHz ? '31kHz High Resolution (VGA / Naomi / Dreamcast)' :
                   'Out of Safe Standard Deflection Range!'}
                </div>
                <div className="text-[11px] opacity-80 mt-0.5 font-sans leading-relaxed">
                  {is15kHz && 'Within optimal Nanao MS8/MS9, Sanwa, and Wells Gardner arcade chassis tolerances.'}
                  {is24kHz && 'Requires a tri-sync or dual-frequency arcade monitor chassis capable of 24kHz.'}
                  {is31kHz && 'Standard VGA arcade frequency. Do not feed directly into 15kHz-only CRT chassis.'}
                  {!is15kHz && !is24kHz && !is31kHz && 'Warning: Abnormal horizontal frequency may damage standard arcade monitor deflection coils.'}
                </div>
              </div>
            </div>
          </div>
        </div>

      </div>

      {/* Generated Modeline Display */}
      <div className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-2">
        <div className="flex items-center justify-between">
          <span className="text-xs font-semibold text-neutral-300 font-mono">
            Generated Linux XFree86 / SwitchRes Modeline:
          </span>
          <button
            onClick={copyModeline}
            className="flex items-center gap-1.5 px-3 py-1 bg-amber-400 hover:bg-amber-300 text-neutral-950 text-xs font-bold rounded transition-colors cursor-pointer"
          >
            {copied ? <Check className="w-3.5 h-3.5" /> : <Copy className="w-3.5 h-3.5" />}
            <span>{copied ? 'Copied!' : 'Copy Modeline'}</span>
          </button>
        </div>

        <div className="p-3 bg-neutral-950 rounded-lg border border-neutral-800 text-amber-300 font-mono text-xs break-all select-all">
          Modeline {modelineString}
        </div>
      </div>
    </div>
  );
};
