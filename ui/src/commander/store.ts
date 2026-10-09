// The objectives of the rig: the one this page runs, and whether any runs.

import { create } from 'zustand';
import { onConnected, onDisconnected, ros } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { cancelAll, followObjective, objectiveRuns, runObjective, type RunResult } from './commander';

interface CommanderState {
  /** An objective runs, sent by this page or by anyone else, e.g. the gamepad. */
  busy: boolean;
  /**
   * The page has heard from the commander whether an objective runs: false
   * until its latched objective came after a connection, see objectiveRuns().
   * Not knowing counts as running for Stop, so that it is never off while the
   * robot might move.
   */
  known: boolean;
  /** The objective this page runs, if any. */
  running?: string;
  /** The objective running, whoever sent it, as the commander says: "" when none. */
  objective: string;
  /** How the last objective of this page ended, while it failed. */
  failure?: string;

  run(objective: string, payload?: string): Promise<RunResult>;
  stop(): Promise<void>;
}

export const useCommander = create<CommanderState>((set) => ({
  busy: false,
  known: false,
  objective: '',

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
    stopFollowing = followObjective(ros(), (objective) => useCommander.setState({ objective, ...objectiveRuns(objective) }));
  };
  follow();
  // A new connection, e.g. to another rosbridge in the settings, gets its own subscription.
  const unsubscribe = onConnected(follow);
  const unsubscribeLost = onDisconnected(() =>
    useCommander.setState({ objective: '', ...objectiveRuns(undefined) }));
  return () => {
    stopFollowing();
    unsubscribe();
    unsubscribeLost();
  };
}
