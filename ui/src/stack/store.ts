// A focus stack: the two marks of the rail, the plan, and the run. Marking and
// shooting are tasks of the rig, the objectives MarkNear, MarkFar and
// FocusStack: the rig saves the marks itself, in its state file, and
// FocusStack reads them there. The page only shows where the rail was when it
// marked, read from the joint states, and keeps the plan of this browser.
//
// The plan is in turns of the motors, which the page shows everywhere; the
// payload of FocusStack is in radians. Degrees of the stage need its gear
// ratio, not measured yet (TODO.md 14).

import { create } from 'zustand';
import { camera } from '../camera/store';
import { useCommander } from '../commander/store';
import { onConnected, ros } from '../ros/connection';
import type { RunResult } from '../commander/commander';
import { DEFAULT_PLAN, payloadOf, totalShots, TURN, type StackPlan } from './plan';

/** The rail, joint2, and the rotary stage, joint1. */
const RAIL = 'joint2';
const KEY = 'stepit-ui.stack';

export type End = 'near' | 'far';

interface StackState extends StackPlan {
  /** Where the rail was when this page marked each end, in turns of its motor. */
  near?: number;
  far?: number;
  /** The end being marked. */
  marking?: End;
  /** How far the rail travels per turn of its motor, in mm: mm_per_turn.joint2 of the commander, if set. */
  mmPerTurn?: number;
  /** While FocusStack runs from this page: the pictures taken so far, of how many. */
  progress?: { taken: number; total: number };

  setPlan(change: Partial<StackPlan>): void;
  mark(end: End): Promise<RunResult>;
  start(): Promise<RunResult>;
}

interface Saved extends StackPlan {
  near?: number;
  far?: number;
}

function load(): Saved {
  try {
    return { ...DEFAULT_PLAN, ...JSON.parse(localStorage.getItem(KEY) ?? '{}') };
  } catch {
    return DEFAULT_PLAN;
  }
}

function save(state: Saved) {
  const { shots, stageFrom, stageTo, angles, near, far } = state;
  try {
    localStorage.setItem(KEY, JSON.stringify({ shots, stageFrom, stageTo, angles, near, far }));
  } catch { /* The plan then lasts until the page is reloaded. */ }
}

interface JointState {
  name: string[];
  position: number[];
}

/** Where the rail is, in turns, from the next joint state. */
function readRail(timeout = 2000): Promise<number | undefined> {
  return new Promise((resolve) => {
    let stop = () => {};
    const timer = setTimeout(() => {
      stop();
      resolve(undefined);
    }, timeout);
    stop = ros().subscribe<JointState>('/joint_states', 'sensor_msgs/msg/JointState', (state) => {
      const index = state.name.indexOf(RAIL);
      if (index < 0) return;
      clearTimeout(timer);
      stop();
      resolve(state.position[index] / TURN);
    });
  });
}

export const useStack = create<StackState>((set, get) => ({
  ...load(),

  setPlan(change) {
    set(change);
    save(get());
  },

  async mark(end) {
    set({ marking: end });
    try {
      const result = await useCommander.getState().run(end === 'near' ? 'MarkNear' : 'MarkFar');
      if (result.ok) {
        // The rail stands still while marked: the joint state shows what the rig saved.
        const position = await readRail();
        set({ [end]: position });
        save(get());
      }
      return result;
    } finally {
      set({ marking: undefined });
    }
  },

  async start() {
    const plan = get();
    const total = totalShots(plan);
    set({ progress: { taken: 0, total } });
    // Every picture of the camera while the stack runs is one of its shots.
    const stop = camera().onPicture(() =>
      set((s) => (s.progress ? { progress: { ...s.progress, taken: s.progress.taken + 1 } } : {})),
    );
    try {
      return await useCommander.getState().run('FocusStack', payloadOf(plan));
    } finally {
      stop();
      set({ progress: undefined });
    }
  },
}));

interface ParameterValue {
  type: number;
  integer_value: number;
  double_value: number;
}

/** The rail's mm_per_turn, from the commander's parameters, which rig.yaml sets. */
async function readMmPerTurn() {
  try {
    const response = await ros().callService<{ values: ParameterValue[] }>(
      '/stepit_server/get_parameters', 'rcl_interfaces/srv/GetParameters', { names: [`mm_per_turn.${RAIL}`] });
    const value = response.values[0];
    // 2 is an integer, 3 a double; 0 is a parameter that is not set.
    const mm = value?.type === 3 ? value.double_value : value?.type === 2 ? value.integer_value : undefined;
    useStack.setState({ mmPerTurn: mm || undefined });
  } catch {
    // No commander yet: the next connection asks again, and the rail shows turns meanwhile.
  }
}

/** Reads the rail's ratio now and on every connection. Returns a function that stops it. */
export function followStack(): () => void {
  void readMmPerTurn();
  return onConnected(() => void readMmPerTurn());
}
