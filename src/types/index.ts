export type ConsoleSystem = 'ps2' | 'gamecube' | 'wii' | 'naomi' | 'model2' | 'saturn';

export interface GameItem {
  id: string;
  title: string;
  system: ConsoleSystem;
  systemName: string;
  romName: string;
  romPath: string;
  emulatorId: string;
  videoMode: string; // e.g. "15kHz 240p @ 60Hz"
  resolution: string; // "640x224", "640x448", etc.
  modeline: string;
  description: string;
  year: number;
  genre: string;
  screenshotUrl?: string;
  bannerColor: string;
}

export interface EmulatorProfile {
  id: string;
  name: string;
  system: ConsoleSystem;
  executableName: string;
  defaultExecutablePath: string;
  commandTemplate: string; // e.g. "{exe} -batch -fullscreen -elf \"{rom}\""
  videoPipeline: 'Groovy_MiSTer D3D9' | 'Groovy_MiSTer Vulkan' | 'SwitchRes Direct' | 'Raw Framebuffer UDP';
  killMethod: 'taskkill' | 'SIGTERM' | 'SIGKILL' | 'graceful_window';
  notes: string;
}

export type MiSTerState = 'MENU_IDLE' | 'SENDING_UDP' | 'LOADING_CORE' | 'STREAMING_ACTIVE' | 'KILLING_PROCESS';

export interface PacketLog {
  id: string;
  timestamp: string;
  type: 'LAUNCH' | 'KILL' | 'STATUS' | 'PING' | 'CATALOG_GET' | 'ACK';
  direction: 'MISTER_TO_PC' | 'PC_TO_MISTER';
  protocol: 'UDP' | 'TCP';
  port: number;
  payload: string;
  latencyMs: number;
  status: 'DELIVERED' | 'PROCESSED' | 'PENDING' | 'REJECTED';
}

export interface ModelinePreset {
  name: string;
  system: string;
  hFreq: string;
  dotClock: number; // MHz
  hActive: number;
  hFront: number;
  hSync: number;
  hBack: number;
  vActive: number;
  vFront: number;
  vSync: number;
  vBack: number;
  interlaced: boolean;
  refreshRate: number;
}
