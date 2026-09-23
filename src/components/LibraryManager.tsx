import React, { useState } from 'react';
import { GameItem, EmulatorProfile, ConsoleSystem } from '../types';
import { Plus, Search, Filter, Trash2, Edit3, Check, X, Download, Server, Gamepad2, Settings } from 'lucide-react';

interface LibraryManagerProps {
  games: GameItem[];
  setGames: React.Dispatch<React.SetStateAction<GameItem[]>>;
  emulators: EmulatorProfile[];
  setEmulators: React.Dispatch<React.SetStateAction<EmulatorProfile[]>>;
  onLaunchGame: (game: GameItem) => void;
}

export const LibraryManager: React.FC<LibraryManagerProps> = ({
  games,
  setGames,
  emulators,
  setEmulators,
  onLaunchGame
}) => {
  const [searchTerm, setSearchTerm] = useState('');
  const [selectedSystem, setSelectedSystem] = useState<string>('all');
  const [activeTab, setActiveTab] = useState<'games' | 'emulators' | 'json'>('games');
  const [isAddModalOpen, setIsAddModalOpen] = useState(false);

  // New Game Form State
  const [newTitle, setNewTitle] = useState('');
  const [newSystem, setNewSystem] = useState<ConsoleSystem>('ps2');
  const [newRomName, setNewRomName] = useState('');
  const [newRomPath, setNewRomPath] = useState('C:\\Games\\PS2\\');
  const [newResolution, setNewResolution] = useState('640x224');
  const [newVideoMode, setNewVideoMode] = useState('15kHz 240p @ 60Hz');
  const [newDescription, setNewDescription] = useState('');
  const [newGenre, setNewGenre] = useState('Fighting (2D)');

  const systemNames: Record<ConsoleSystem, string> = {
    mame: 'GroovyMAME Arcade (15kHz)',
    ps2: 'Sony PlayStation 2',
    gamecube: 'Nintendo GameCube',
    wii: 'Nintendo Wii',
    naomi: 'Sega Naomi Arcade',
    model2: 'Sega Model 2 Arcade',
    saturn: 'Sega Saturn'
  };

  const filteredGames = games.filter(g => {
    const matchesSearch = g.title.toLowerCase().includes(searchTerm.toLowerCase()) ||
                          g.romName.toLowerCase().includes(searchTerm.toLowerCase()) ||
                          g.genre.toLowerCase().includes(searchTerm.toLowerCase());
    const matchesSystem = selectedSystem === 'all' || g.system === selectedSystem;
    return matchesSearch && matchesSystem;
  });

  const handleAddGame = (e: React.FormEvent) => {
    e.preventDefault();
    if (!newTitle.trim() || !newRomName.trim()) return;

    const gameId = `${newSystem}_${newTitle.toLowerCase().replace(/[^a-z0-9]/g, '_')}`;
    const newGame: GameItem = {
      id: gameId,
      title: newTitle.trim(),
      system: newSystem,
      systemName: systemNames[newSystem],
      romName: newRomName.trim(),
      romPath: newRomPath.endsWith('\\') ? `${newRomPath}${newRomName.trim()}` : `${newRomPath}\\${newRomName.trim()}`,
      emulatorId: newSystem === 'ps2' ? 'pcsx2' : newSystem === 'gamecube' ? 'dolphin' : newSystem === 'wii' ? 'dolphin_wii' : 'flycast',
      videoMode: newVideoMode,
      resolution: newResolution,
      modeline: `"640x224_60" 12.80 640 664 728 800 224 236 239 262 -hsync -vsync`,
      description: newDescription.trim() || `${newTitle} configured for Groovy_MiSTer 15kHz CRT playback.`,
      year: 2004,
      genre: newGenre,
      bannerColor: 'from-amber-950/80 to-neutral-900'
    };

    setGames(prev => [newGame, ...prev]);
    setIsAddModalOpen(false);
    // Reset form
    setNewTitle('');
    setNewRomName('');
  };

  const handleDeleteGame = (id: string) => {
    setGames(prev => prev.filter(g => g.id !== id));
  };

  const handleExportJson = () => {
    const data = {
      systems: Object.keys(systemNames),
      games: games,
      emulators: emulators
    };
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'games_catalog.json';
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="space-y-6 max-w-7xl mx-auto py-4">
      {/* Title & Navigation */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 border-b border-neutral-800 pb-4">
        <div>
          <h2 className="text-xl font-bold tracking-tight text-white flex items-center gap-2">
            <span>Games & Emulator Profiles</span>
            <span className="text-xs font-mono font-normal px-2 py-0.5 bg-neutral-900 border border-neutral-800 rounded text-neutral-400">
              {games.length} Titles Registered
            </span>
          </h2>
          <p className="text-xs text-neutral-400 mt-1">
            Manage your PC-hosted game catalog, launch parameters, and video timing definitions synchronized with MiSTer.
          </p>
        </div>

        {/* Segmented Tab Bar */}
        <div className="flex items-center gap-1.5 p-1 bg-neutral-900 border border-neutral-800 rounded-lg">
          <button
            onClick={() => setActiveTab('games')}
            className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors cursor-pointer ${
              activeTab === 'games'
                ? 'bg-neutral-800 text-white shadow-sm'
                : 'text-neutral-400 hover:text-neutral-200'
            }`}
          >
            Game Library
          </button>
          <button
            onClick={() => setActiveTab('emulators')}
            className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors cursor-pointer ${
              activeTab === 'emulators'
                ? 'bg-neutral-800 text-white shadow-sm'
                : 'text-neutral-400 hover:text-neutral-200'
            }`}
          >
            Emulators
          </button>
          <button
            onClick={() => setActiveTab('json')}
            className={`px-3 py-1.5 text-xs font-medium rounded-md transition-colors cursor-pointer ${
              activeTab === 'json'
                ? 'bg-neutral-800 text-white shadow-sm'
                : 'text-neutral-400 hover:text-neutral-200'
            }`}
          >
            catalog.json
          </button>
        </div>
      </div>

      {/* TAB 1: GAMES LIST */}
      {activeTab === 'games' && (
        <div className="space-y-4">
          {/* Filter Bar & Search */}
          <div className="flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-3">
            <div className="flex items-center gap-2 flex-1 max-w-md">
              <div className="relative flex-1">
                <Search className="w-4 h-4 absolute left-3 top-1/2 -translate-y-1/2 text-neutral-500" />
                <input
                  type="text"
                  placeholder="Search games, ROM filenames, genres..."
                  value={searchTerm}
                  onChange={(e) => setSearchTerm(e.target.value)}
                  className="w-full bg-neutral-900 border border-neutral-800 rounded-lg pl-9 pr-4 py-2 text-xs text-white placeholder-neutral-500 focus:outline-none focus:border-amber-400"
                />
              </div>

              <select
                value={selectedSystem}
                onChange={(e) => setSelectedSystem(e.target.value)}
                className="bg-neutral-900 border border-neutral-800 rounded-lg px-3 py-2 text-xs text-neutral-300 focus:outline-none focus:border-amber-400"
              >
                <option value="all">All Systems</option>
                <option value="mame">GroovyMAME Arcade</option>
                <option value="ps2">PlayStation 2</option>
                <option value="gamecube">GameCube</option>
                <option value="wii">Wii</option>
                <option value="naomi">Sega Naomi</option>
                <option value="model2">Sega Model 2</option>
                <option value="saturn">Sega Saturn</option>
              </select>
            </div>

            <div className="flex items-center gap-2">
              <button
                onClick={handleExportJson}
                className="px-3 py-2 bg-neutral-900 hover:bg-neutral-800 border border-neutral-800 rounded-lg text-xs font-medium text-neutral-300 flex items-center gap-1.5 transition-colors cursor-pointer"
              >
                <Download className="w-3.5 h-3.5" />
                <span>Export catalog.json</span>
              </button>

              <button
                onClick={() => setIsAddModalOpen(true)}
                className="px-3 py-2 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded-lg text-xs flex items-center gap-1.5 transition-colors cursor-pointer"
              >
                <Plus className="w-3.5 h-3.5" />
                <span>Add Game Title</span>
              </button>
            </div>
          </div>

          {/* Games Table */}
          <div className="bg-neutral-900/60 border border-neutral-800 rounded-xl overflow-hidden">
            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs">
                <thead>
                  <tr className="border-b border-neutral-800 text-neutral-400 bg-neutral-900/80 font-mono">
                    <th className="py-3 px-4 font-medium">Title & System</th>
                    <th className="py-3 px-4 font-medium">ROM File</th>
                    <th className="py-3 px-4 font-medium">Video Mode</th>
                    <th className="py-3 px-4 font-medium">Pipeline</th>
                    <th className="py-3 px-4 font-medium text-right">Actions</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-neutral-800/60 font-mono">
                  {filteredGames.length === 0 ? (
                    <tr>
                      <td colSpan={5} className="py-8 text-center text-neutral-500 font-sans">
                        No titles found matching your search.
                      </td>
                    </tr>
                  ) : (
                    filteredGames.map((game) => (
                      <tr key={game.id} className="hover:bg-neutral-800/40 transition-colors group">
                        <td className="py-3 px-4">
                          <div className="font-semibold text-white font-sans text-sm">{game.title}</div>
                          <div className="text-[11px] text-neutral-400 font-sans flex items-center gap-1.5 mt-0.5">
                            <span>{game.systemName}</span>
                            <span>·</span>
                            <span>{game.genre}</span>
                            <span>·</span>
                            <span>{game.year}</span>
                          </div>
                        </td>

                        <td className="py-3 px-4 text-neutral-300">
                          <div className="text-amber-300 truncate max-w-[200px]">{game.romName}</div>
                          <div className="text-[10px] text-neutral-400 truncate max-w-[240px]">{game.romPath}</div>
                        </td>

                        <td className="py-3 px-4">
                          <div className="text-emerald-400">{game.videoMode}</div>
                          <div className="text-[10px] text-neutral-400">{game.resolution}</div>
                        </td>

                        <td className="py-3 px-4 text-neutral-400">
                          <span className="px-2 py-0.5 bg-neutral-950 border border-neutral-800 rounded text-[10px]">
                            {game.emulatorId}
                          </span>
                        </td>

                        <td className="py-3 px-4 text-right">
                          <div className="flex items-center justify-end gap-2">
                            <button
                              onClick={() => onLaunchGame(game)}
                              className="px-2.5 py-1 bg-amber-400/10 hover:bg-amber-400/20 text-amber-300 border border-amber-400/30 rounded text-[11px] font-sans font-medium transition-colors cursor-pointer"
                            >
                              Test Launch
                            </button>
                            <button
                              onClick={() => handleDeleteGame(game.id)}
                              className="p-1.5 text-neutral-500 hover:text-rose-400 hover:bg-neutral-800 rounded transition-colors cursor-pointer"
                              title="Delete Game"
                            >
                              <Trash2 className="w-3.5 h-3.5" />
                            </button>
                          </div>
                        </td>
                      </tr>
                    ))
                  )}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      )}

      {/* TAB 2: EMULATOR PROFILES */}
      {activeTab === 'emulators' && (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {emulators.map((emu) => (
            <div key={emu.id} className="p-5 bg-neutral-900 border border-neutral-800 rounded-xl space-y-3">
              <div className="flex items-center justify-between">
                <div>
                  <h4 className="text-sm font-bold text-white">{emu.name}</h4>
                  <div className="text-xs text-neutral-400 font-mono mt-0.5 uppercase">
                    System: {emu.system} · {emu.videoPipeline}
                  </div>
                </div>
                <span className="text-[10px] font-mono px-2 py-0.5 bg-neutral-950 border border-neutral-800 rounded text-amber-400">
                  {emu.killMethod}
                </span>
              </div>

              <div className="space-y-1.5 text-xs font-mono">
                <div className="text-neutral-400">Default Executable:</div>
                <div className="bg-neutral-950 p-2 rounded border border-neutral-800/80 text-neutral-300 break-all text-[11px]">
                  {emu.defaultExecutablePath}
                </div>

                <div className="text-neutral-400 pt-1">Command-line Execution Template:</div>
                <div className="bg-neutral-950 p-2 rounded border border-neutral-800/80 text-amber-300 break-all text-[11px]">
                  {emu.commandTemplate}
                </div>
              </div>

              <div className="text-xs text-neutral-400 pt-1 border-t border-neutral-800/80 leading-relaxed font-sans">
                {emu.notes}
              </div>
            </div>
          ))}
        </div>
      )}

      {/* TAB 3: JSON CATALOG PREVIEW */}
      {activeTab === 'json' && (
        <div className="p-4 bg-neutral-900 border border-neutral-800 rounded-xl space-y-3">
          <div className="flex items-center justify-between">
            <span className="text-xs font-mono text-neutral-400">
              HTTP Endpoint: <code className="text-amber-300">GET /catalog.json</code>
            </span>
            <button
              onClick={handleExportJson}
              className="px-3 py-1 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded text-xs transition-colors cursor-pointer"
            >
              Download JSON
            </button>
          </div>
          <pre className="p-4 bg-neutral-950 rounded-lg border border-neutral-800 font-mono text-xs text-emerald-400 overflow-x-auto max-h-[480px]">
            {JSON.stringify({ systems: Object.keys(systemNames), games, emulators }, null, 2)}
          </pre>
        </div>
      )}

      {/* ADD GAME MODAL */}
      {isAddModalOpen && (
        <div className="fixed inset-0 bg-black/80 backdrop-blur-xs z-50 flex items-center justify-center p-4">
          <div className="bg-neutral-900 border border-neutral-800 rounded-xl max-w-lg w-full p-6 space-y-4 shadow-2xl">
            <div className="flex items-center justify-between border-b border-neutral-800 pb-3">
              <h3 className="text-base font-bold text-white">Add Heavy 3D / Arcade Title</h3>
              <button
                onClick={() => setIsAddModalOpen(false)}
                className="text-neutral-400 hover:text-white cursor-pointer"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <form onSubmit={handleAddGame} className="space-y-3 text-xs">
              <div>
                <label className="block text-neutral-300 font-medium mb-1">Game Title</label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Arcana Heart, Melty Blood Actress Again"
                  value={newTitle}
                  onChange={(e) => setNewTitle(e.target.value)}
                  className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-neutral-300 font-medium mb-1">Console System</label>
                  <select
                    value={newSystem}
                    onChange={(e) => {
                      const sys = e.target.value as ConsoleSystem;
                      setNewSystem(sys);
                      if (sys === 'mame') setNewRomPath('C:\\Emulators\\GroovyMAME\\roms\\');
                      else if (sys === 'ps2') setNewRomPath('C:\\Games\\PS2\\');
                      else if (sys === 'gamecube') setNewRomPath('C:\\Games\\GameCube\\');
                      else if (sys === 'wii') setNewRomPath('C:\\Games\\Wii\\');
                      else if (sys === 'naomi') setNewRomPath('C:\\Games\\Arcade\\Naomi\\');
                      else if (sys === 'model2') setNewRomPath('C:\\Games\\Arcade\\Model2\\');
                    }}
                    className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400"
                  >
                    <option value="mame">GroovyMAME Arcade (15kHz)</option>
                    <option value="ps2">Sony PlayStation 2</option>
                    <option value="gamecube">Nintendo GameCube</option>
                    <option value="wii">Nintendo Wii</option>
                    <option value="naomi">Sega Naomi Arcade</option>
                    <option value="model2">Sega Model 2 Arcade</option>
                    <option value="saturn">Sega Saturn</option>
                  </select>
                </div>

                <div>
                  <label className="block text-neutral-300 font-medium mb-1">Genre</label>
                  <input
                    type="text"
                    value={newGenre}
                    onChange={(e) => setNewGenre(e.target.value)}
                    className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400"
                  />
                </div>
              </div>

              <div>
                <label className="block text-neutral-300 font-medium mb-1">ROM / ISO Filename</label>
                <input
                  type="text"
                  required
                  placeholder="e.g. ArcanaHeart.iso or vf4ft.zip"
                  value={newRomName}
                  onChange={(e) => setNewRomName(e.target.value)}
                  className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400 font-mono"
                />
              </div>

              <div>
                <label className="block text-neutral-300 font-medium mb-1">Full PC ROM Path</label>
                <input
                  type="text"
                  required
                  value={newRomPath}
                  onChange={(e) => setNewRomPath(e.target.value)}
                  className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400 font-mono text-[11px]"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-neutral-300 font-medium mb-1">Video Timing Mode</label>
                  <select
                    value={newVideoMode}
                    onChange={(e) => setNewVideoMode(e.target.value)}
                    className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400"
                  >
                    <option value="15kHz 240p @ 60Hz">15kHz 240p @ 60Hz</option>
                    <option value="15kHz 480i @ 60Hz">15kHz 480i @ 60Hz</option>
                    <option value="24kHz Med-Res @ 60Hz">24kHz Med-Res @ 60Hz</option>
                    <option value="31kHz VGA @ 60Hz">31kHz VGA @ 60Hz</option>
                  </select>
                </div>

                <div>
                  <label className="block text-neutral-300 font-medium mb-1">Native Resolution</label>
                  <input
                    type="text"
                    value={newResolution}
                    onChange={(e) => setNewResolution(e.target.value)}
                    className="w-full bg-neutral-950 border border-neutral-800 rounded px-3 py-2 text-white focus:outline-none focus:border-amber-400 font-mono"
                  />
                </div>
              </div>

              <div className="pt-3 border-t border-neutral-800 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setIsAddModalOpen(false)}
                  className="px-3 py-1.5 bg-neutral-800 hover:bg-neutral-700 text-neutral-300 rounded font-medium cursor-pointer"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-1.5 bg-amber-400 hover:bg-amber-300 text-neutral-950 font-bold rounded cursor-pointer"
                >
                  Save Title
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
};
