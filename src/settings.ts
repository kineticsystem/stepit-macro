// Preferences of this browser, kept in localStorage. Add new settings to
// Settings and DEFAULTS, and a control to components/SettingsMenu.tsx.
//
// The servers default to the host that served the page: the rig, whether the
// page is opened on its desktop or on a tablet on the network.

import { create } from 'zustand';

export type Theme = 'auto' | 'light' | 'dark';

export interface Settings {
  theme: Theme;
  /** The rig's rosbridge, e.g. ws://stepit-macro:9090; empty for port 9090 of the page's host. */
  rosbridgeUrl: string;
  /** The camera's web_video_server, e.g. http://stepit-macro:8081; empty for port 8081 of the page's host. */
  videoUrl: string;
  /** The camera's web server, which serves the pictures, e.g. http://stepit-macro:8090; empty for port 8090 of the page's host. */
  picturesUrl: string;
  /** The name of the camera node, e.g. /camera. */
  cameraNode: string;
}

/** The camera node of the rig. */
export const DEFAULT_CAMERA_NODE = '/camera';

const DEFAULTS: Settings = { theme: 'auto', rosbridgeUrl: '', videoUrl: '', picturesUrl: '', cameraNode: DEFAULT_CAMERA_NODE };
const KEY = 'stepit-ui.settings';

function load(): Settings {
  try {
    return { ...DEFAULTS, ...JSON.parse(localStorage.getItem(KEY) ?? '{}') };
  } catch {
    return DEFAULTS;
  }
}

interface SettingsState extends Settings {
  update(change: Partial<Settings>): void;
}

export const useSettings = create<SettingsState>((set, get) => ({
  ...load(),
  update(change) {
    set(change);
    const { update: _, ...settings } = { ...get() };
    try {
      localStorage.setItem(KEY, JSON.stringify(settings));
    } catch { /* The settings then last until the page is reloaded. */ }
  },
}));

/** The host that served the page: the rig. */
const pageHost = () => (typeof location === 'undefined' ? '' : location.hostname) || 'localhost';

const trimmed = (url: string) => url.trim().replace(/\/+$/, '');

export function rosbridgeUrl(settings: Pick<Settings, 'rosbridgeUrl'>): string {
  return trimmed(settings.rosbridgeUrl) || `ws://${pageHost()}:9090`;
}

export function videoUrl(settings: Pick<Settings, 'videoUrl'>): string {
  return trimmed(settings.videoUrl) || `http://${pageHost()}:8081`;
}

export function picturesUrl(settings: Pick<Settings, 'picturesUrl'>): string {
  return trimmed(settings.picturesUrl) || `http://${pageHost()}:8090`;
}

/** Sets data-theme on <html>, which styles.css keys the dark palette on. */
function applyTheme() {
  const { theme } = useSettings.getState();
  const resolved = theme === 'auto' ? (systemDark.matches ? 'dark' : 'light') : theme;
  document.documentElement.dataset.theme = resolved;
}

const systemDark = window.matchMedia('(prefers-color-scheme: dark)');
applyTheme();
useSettings.subscribe(applyTheme);
// In Auto mode, follow the system when it switches, e.g. at sunset.
systemDark.addEventListener('change', applyTheme);
