// The objectives of the rig: the one this page runs, and whether any runs.

import { create } from 'zustand';
import { onConnected, ros } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { cancelAll, followObjectives, runObjective, type RunResult } from './commander';

interface CommanderState {
  /** An objective runs, sent by this page or by anyone else, e.g. the gamepad. */
  busy: boolean;
  /** The objective this page runs, if any. */
  running?: string;
  /** How the last objective of this page ended, while it failed. */
  failure?: string;

  run(objective: string, payload?: string): Promise<RunResult>;
  stop(): Promise<void>;
}

export const useCommander = create<CommanderState>((set) => ({
  busy: false,

  async run(objective, payload = '') {
    set({ running: objective, failure: undefined });
    try {
      const result = await runObjective(ros(), objective, payload);
      if (!result.ok) set({ failure: `${objective}: ${result.message}` });
      return result;
    } catch (e) {
      const message = errorMessage(e);
      set({ failure: `${objective}: ${message}` });
      return { ok: false, message };
    } finally {
      set((s) => (s.running === objective ? { running: undefined } : {}));
    }
  },

  async stop() {
    try {
      await cancelAll(ros());
    } catch (e) {
      set({ failure: `Stop: ${errorMessage(e)}` });
    }
  },
}));

/** Keeps busy up to date while the page is open. Returns a function that stops it. */
export function followCommander(): () => void {
  let stopFollowing = () => {};
  const follow = () => {
    stopFollowing();
    stopFollowing = followObjectives(ros(), (busy) => useCommander.setState({ busy }));
  };
  follow();
  // A new connection, e.g. to another rosbridge in the settings, gets its own subscription.
  const unsubscribe = onConnected(follow);
  return () => {
    stopFollowing();
    unsubscribe();
  };
}
