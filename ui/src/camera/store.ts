// The state of the camera: the settings read from the driver, and the live
// view. Both configure the camera, so they go straight to its driver, not
// through the commander.

import { create } from 'zustand';
import { useCommander } from '../commander/store';
import { onConnected, ros, status } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { useSettings } from '../settings';
import { Camera, type CameraSetting } from './camera';

/** How often the settings are read again, to follow changes made on the camera itself. */
const REFRESH_PERIOD = 3000;

interface CameraState {
  settings: CameraSetting[];
  /** Why the settings cannot be read, e.g. the camera is off. */
  error?: string;
  /** The settings being changed, with the value asked for. */
  changing: Record<string, string>;
  /** Why the last change failed, by setting. */
  refused: Record<string, string>;
  streaming: boolean;
  streamError?: string;

  refresh(): Promise<void>;
  change(name: string, value: string): Promise<void>;
  setStreaming(on: boolean): Promise<void>;
}

export const camera = () => new Camera(ros(), useSettings.getState().cameraNode);

export const useCamera = create<CameraState>((set, get) => ({
  settings: [],
  changing: {},
  refused: {},
  streaming: false,

  async refresh() {
    try {
      set({ settings: await camera().getSettings(), error: undefined });
    } catch (e) {
      set({ error: errorMessage(e) });
    }
  },

  async change(name, value) {
    set((s) => ({ changing: { ...s.changing, [name]: value }, refused: without(s.refused, name) }));
    try {
      await camera().setSetting(name, value);
    } catch (e) {
      set((s) => ({ refused: { ...s.refused, [name]: errorMessage(e) } }));
    } finally {
      set((s) => ({ changing: without(s.changing, name) }));
    }
    await get().refresh();
  },

  async setStreaming(on) {
    set({ streaming: on, streamError: undefined });
    try {
      await (on ? camera().startStreaming() : camera().stopStreaming());
    } catch (e) {
      set({ streamError: errorMessage(e) });
    }
  },
}));

function without<T>(record: Record<string, T>, key: string): Record<string, T> {
  const { [key]: _, ...rest } = record;
  return rest;
}

/**
 * Keeps the camera up to date while the page is open: reads the settings now
 * and then, and starts the live view again whenever rosbridge reconnects, if
 * it was on. Returns a function that stops it.
 */
export function followCamera(): () => void {
  const connected = () => {
    void useCamera.getState().refresh();
    // The live view is off when the page opens. Once started, it is started
    // again after a reconnection, e.g. when the rig restarted.
    if (useCamera.getState().streaming) void useCamera.getState().setStreaming(true);
  };
  ros();
  if (status() === 'connected') connected();
  const unsubscribe = onConnected(connected);
  const timer = setInterval(() => {
    const { changing } = useCamera.getState();
    // During a task, e.g. a shot, the driver may be busy downloading a
    // picture, e.g. 29 MB for a RAW: a request would only time out.
    if (status() === 'connected' && Object.keys(changing).length === 0 && !useCommander.getState().busy) {
      void useCamera.getState().refresh();
    }
  }, REFRESH_PERIOD);
  return () => {
    unsubscribe();
    clearInterval(timer);
  };
}
