// The power button's state, for the button and for the top bar: whether the
// rig is switching off, why it refused, and the hint after a mere tap.

import { create } from 'zustand';
import { onConnected, ros } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { powerOff } from './power';

/** How long the hint to hold shows after a tap, in milliseconds. */
const HINT_MS = 2500;

interface PowerState {
  /** power_off said yes: the rig is switching off, and the page will lose it. */
  switchingOff: boolean;
  /** Why the last request failed, e.g. a stack runs, or the system refused. */
  failure?: string;
  /** Shown a moment after a tap: a tablet has no tooltip. */
  hint?: string;
  switchOff(): Promise<void>;
  showHint(): void;
}

let hintTimer: ReturnType<typeof setTimeout> | undefined;

export const usePower = create<PowerState>((set) => ({
  switchingOff: false,
  async switchOff() {
    set({ failure: undefined, hint: undefined });
    try {
      const result = await powerOff(ros());
      set(result.ok ? { switchingOff: true } : { failure: `Power off: ${result.message}` });
    } catch (e) {
      set({ failure: `Power off: ${errorMessage(e)}` });
    }
  },
  showHint() {
    clearTimeout(hintTimer);
    set({ hint: 'Hold the power button for 3 seconds to switch the rig off' });
    hintTimer = setTimeout(() => set({ hint: undefined }), HINT_MS);
  },
}));

/** A new connection is a rig that is on again: forget the last switch off. Returns a function that stops it. */
export function followPower(): () => void {
  return onConnected(() => usePower.setState({ switchingOff: false, failure: undefined }));
}
