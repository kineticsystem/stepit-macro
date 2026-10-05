// The lights of the rig, on a jack of StepIt Freezer, switched by hand: see
// outputs.ts for the lines of each jack.
//
//   /freezer/outputs       the outputs of the board whenever they change; the
//                          last one reaches a late subscriber
//   /freezer/set_outputs   sets all 16 outputs; refused while a shot runs,
//                          which owns them, and which ends with all of them off

import { create } from 'zustand';
import { onConnected, ros } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { LIGHTS_JACK, withLights } from './outputs';

export { LIGHTS_JACK };

interface OutputsMessage {
  outputs: number;
}

interface SetOutputsResponse {
  success: boolean;
  message: string;
}

interface LightsState {
  /** The outputs of the board, once known. */
  outputs?: number;
  switching: boolean;
  error?: string;
  setLights(on: boolean): Promise<void>;
}

export const useLights = create<LightsState>((set, get) => ({
  switching: false,

  async setLights(on) {
    set({ switching: true, error: undefined });
    try {
      const response = await ros().callService<SetOutputsResponse>('/freezer/set_outputs', 'freezer_msgs/srv/SetOutputs',
        { outputs: withLights(get().outputs ?? 0, on) });
      if (!response.success) set({ error: response.message });
    } catch (e) {
      set({ error: errorMessage(e) });
    } finally {
      set({ switching: false });
    }
  },
}));

/** Follows the outputs of the board while the page is open. Returns a function that stops it. */
export function followLights(): () => void {
  let stopFollowing = () => {};
  const follow = () => {
    stopFollowing();
    stopFollowing = ros().subscribe<OutputsMessage>('/freezer/outputs', 'freezer_msgs/msg/Outputs',
      (message) => useLights.setState({ outputs: message.outputs }),
      { reliability: 'reliable', durability: 'transient_local', history: 'keep_last', depth: 1 });
  };
  follow();
  const unsubscribe = onConnected(follow);
  return () => {
    stopFollowing();
    unsubscribe();
  };
}
