// Switching the rig's computer off: the service of the node power_off, which
// refuses while a stack is shot. The page holds a button for POWER_HOLD_MS
// before it asks, so that a touch alone never switches the rig off.

import type { Rosbridge } from '../ros/rosbridge';

export const POWER_OFF = '/power_off/power_off';
export const TRIGGER = 'std_srvs/srv/Trigger';
/** How long the power button must be held, in milliseconds. */
export const POWER_HOLD_MS = 3000;
/** The objectives power_off refuses to cut short: its parameter refuse_during in rig.yaml. */
export const STACKS = ['FocusStack', 'Stack'];

export interface PowerResult {
  /** The rig is switching off. */
  ok: boolean;
  /** What power_off answered: why not, when it refused. */
  message: string;
}

export async function powerOff(ros: Rosbridge): Promise<PowerResult> {
  const response = await ros.callService<{ success: boolean; message: string }>(POWER_OFF, TRIGGER);
  return { ok: response.success, message: response.message };
}

/**
 * Why the power button is off, or nothing when it can be held. power_off
 * refuses a stack itself: the page only says so before the hold.
 */
export function powerProblem(connected: boolean, objective: string, switchingOff: boolean): string | undefined {
  if (switchingOff) return 'The rig is switching off';
  if (!connected) return 'Not connected to the rig';
  if (STACKS.includes(objective)) return `${objective} is running: stop it first`;
  return undefined;
}
