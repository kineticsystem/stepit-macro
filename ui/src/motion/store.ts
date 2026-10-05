// Driving the joints by hand from the sliders: joint1, the rotary stage, and
// joint2, the rail.
//
// The sliders drive the velocity controller, which runs only once the robot is
// handed to the user by the objective ActivateTeleop, as the gamepad's stop
// button does. Manual drive ends by handing the robot back to the trajectory
// controller, the one the rig starts with, through the objective
// ActivateController. The page sees whether it runs from the controller manager,
// which any objective may change, e.g. a move, which takes the trajectory
// controller instead.

import { create } from 'zustand';
import { useCommander } from '../commander/store';
import { onConnected, onDisconnected, ros, status } from '../ros/connection';
import { MAX_TURNS_PER_SECOND } from './axis';
import { JoyPublisher } from './joy';

/**
 * The slider of each joint, by its axis on /ui/joy, and its speed at the ends,
 * in turns per second. The speed is set in the rig, by the scale of the joint
 * in the section ui_teleop of rig.yaml: this one only labels the slider. Keep
 * them in step.
 */
export const SLIDERS = [
  // A quarter of the motors' limit: the subject turns on it.
  { axis: 0, joint: 'joint1', label: 'Rotary stage', maxTurnsPerSecond: MAX_TURNS_PER_SECOND / 4 },
  { axis: 1, joint: 'joint2', label: 'Rail', maxTurnsPerSecond: MAX_TURNS_PER_SECOND },
] as const;

const CONTROLLER = 'velocity_controller';
/** The controller that drives the robot when manual drive is off: the one the rig starts with. */
const DEFAULT_CONTROLLER = 'joint_trajectory_controller';
/** How often the page asks the controller manager which controllers run. */
const POLL_PERIOD = 2000;

interface ListControllersResponse {
  controller: { name: string; state: string }[];
}

interface MotionState {
  /** The velocity controller runs: the sliders drive the joints. */
  enabled: boolean;
  /** The axis of each slider, from -1 to 1. */
  axes: number[];
  enable(): Promise<void>;
  /** Releases the sliders, and hands the robot back to the trajectory controller. */
  disable(): Promise<void>;
  setAxis(axis: number, value: number): void;
  release(): void;
}

let publisher: JoyPublisher | undefined;
const joy = () => (publisher ??= new JoyPublisher((topic, type, message) => ros().publish(topic, type, message), SLIDERS.length));

export const useMotion = create<MotionState>((set, get) => ({
  enabled: false,
  axes: SLIDERS.map(() => 0),

  async enable() {
    const result = await useCommander.getState().run('ActivateTeleop');
    if (result.ok) set({ enabled: true });
  },

  async disable() {
    get().release();
    const result = await useCommander.getState().run('ActivateController', `{controllers: ${DEFAULT_CONTROLLER}}`);
    if (result.ok) set({ enabled: false });
  },

  setAxis(axis, value) {
    if (!get().enabled && value !== 0) return;
    joy().set(axis, value);
    set((s) => ({ axes: s.axes.map((v, i) => (i === axis ? value : v)) }));
  },

  release() {
    joy().release();
    set({ axes: SLIDERS.map(() => 0) });
  },
}));

async function refresh() {
  if (status() !== 'connected') return;
  try {
    const response = await ros().callService<ListControllersResponse>('/controller_manager/list_controllers',
      'controller_manager_msgs/srv/ListControllers');
    const enabled = response.controller.some((c) => c.name === CONTROLLER && c.state === 'active');
    useMotion.setState({ enabled });
    if (!enabled) useMotion.getState().release();
  } catch {
    // The robot is not running: the sliders cannot drive it.
    useMotion.setState({ enabled: false });
  }
}

/**
 * Follows the controllers while the page is open, and stops the sliders when
 * the page is hidden, e.g. the tablet goes to sleep with a finger on one.
 * Disables them while disconnected, when their messages would be dropped:
 * they come back with the controllers, read again on the next connection.
 * Returns a function that stops it.
 */
export function followMotion(): () => void {
  void refresh();
  const timer = setInterval(() => void refresh(), POLL_PERIOD);
  const unsubscribe = onConnected(() => void refresh());
  const unsubscribeLost = onDisconnected(() => {
    useMotion.setState({ enabled: false });
    useMotion.getState().release();
  });
  const hidden = () => document.hidden && useMotion.getState().release();
  document.addEventListener('visibilitychange', hidden);
  return () => {
    clearInterval(timer);
    unsubscribe();
    unsubscribeLost();
    document.removeEventListener('visibilitychange', hidden);
    useMotion.getState().release();
  };
}
