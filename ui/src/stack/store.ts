// A focus stack: the ends of the rail and of the stage, the plan, and the run.
//
// The rail's ends are marked by the rig, the objectives MarkNear and MarkFar,
// which save them in its state file, where FocusStack reads them: the page
// only shows where the rail was when it marked. The stage's ends are the
// angles of rig.yaml, focus_stack.stage_from and stage_to, in degrees from
// where the stage is when the stack starts; a Mark at the stage's slider
// overrides one with where the stage is, in this page alone, until the page
// connects to the rig again, e.g. after a restart. Start turns them into
// angles from where the stage is then, for FocusStack.
//
// The rail shows in millimetres, with the commander's mm_per_turn.joint2; the
// stage in degrees, with deg_per_turn.joint1.

import { create } from 'zustand';
import { camera } from '../camera/store';
import { useCommander } from '../commander/store';
import { onConnected, ros } from '../ros/connection';
import type { RunResult } from '../commander/commander';
import { DEFAULT_PLAN, payloadOf, stageAngle, totalShots, TURN, type StackPlan, type StageEnd } from './plan';

/** The rail, joint2, and the rotary stage, joint1. */
const RAIL = 'joint2';
const STAGE = 'joint1';
const KEY = 'stepit-ui.stack';

export type End = 'near' | 'far';

/** What this browser keeps: the counts and the rail's marks as shown. */
interface Saved {
  shots: number;
  angles: number;
  near?: number;
  far?: number;
}

interface StackState extends Saved {
  /** The stage's angles of rig.yaml, in degrees from where it is. */
  stageConfig: { from: number; to: number };
  /** The stage's ends marked on this page, positions of its motor, in radians. */
  stageMarks: Partial<Record<StageEnd, number>>;
  /** Where the stage's motor is now, in radians, from the joint states. */
  stagePosition?: number;
  /** The rail end being marked. */
  marking?: End;
  /** How far the rail travels per turn of its motor, in mm: mm_per_turn.joint2 of the commander, if set. */
  mmPerTurn?: number;
  /** How far the stage turns per turn of its motor, in degrees: deg_per_turn.joint1, if set. */
  degPerTurn?: number;
  /** While FocusStack runs from this page: the pictures taken so far, of how many. */
  progress?: { taken: number; total: number };

  setPlan(change: Partial<Pick<Saved, 'shots' | 'angles'>>): void;
  mark(end: End): Promise<RunResult>;
  markStage(end: StageEnd): void;
  start(): Promise<RunResult>;
}

function load(): Saved {
  const defaults = { shots: DEFAULT_PLAN.shots, angles: DEFAULT_PLAN.angles };
  try {
    const { shots, angles, near, far } = JSON.parse(localStorage.getItem(KEY) ?? '{}');
    return { ...defaults, ...(shots && { shots }), ...(angles && { angles }), near, far };
  } catch {
    return defaults;
  }
}

function save({ shots, angles, near, far }: Saved) {
  try {
    localStorage.setItem(KEY, JSON.stringify({ shots, angles, near, far }));
  } catch { /* The plan then lasts until the page is reloaded. */ }
}

/** The angle of an end of the stage, in degrees from where it is now. */
export function stageAngleOf(state: StackState, end: StageEnd): number | undefined {
  return stageAngle(state.stageConfig[end], state.stageMarks[end], state.stagePosition, state.degPerTurn);
}

/** The plan FocusStack is asked for now, the stage's ends from where it is. */
export function planOf(state: StackState): StackPlan & { near?: number; far?: number } {
  return {
    shots: state.shots,
    angles: state.angles,
    stageFrom: stageAngleOf(state, 'from') ?? NaN,
    stageTo: stageAngleOf(state, 'to') ?? NaN,
    near: state.near,
    far: state.far,
  };
}

interface JointState {
  name: string[];
  position: number[];
}

/** Where a joint is, in radians, from the next joint state. */
function readJoint(joint: string, timeout = 2000): Promise<number | undefined> {
  return new Promise((resolve) => {
    let stop = () => {};
    const timer = setTimeout(() => {
      stop();
      resolve(undefined);
    }, timeout);
    stop = ros().subscribe<JointState>('/joint_states', 'sensor_msgs/msg/JointState', (state) => {
      const index = state.name.indexOf(joint);
      if (index < 0) return;
      clearTimeout(timer);
      stop();
      resolve(state.position[index]);
    });
  });
}

export const useStack = create<StackState>((set, get) => ({
  ...load(),
  stageConfig: { from: DEFAULT_PLAN.stageFrom, to: DEFAULT_PLAN.stageTo },
  stageMarks: {},

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
        const position = await readJoint(RAIL);
        set({ [end]: position === undefined ? undefined : position / TURN });
        save(get());
      }
      return result;
    } finally {
      set({ marking: undefined });
    }
  },

  markStage(end) {
    const position = get().stagePosition;
    if (position !== undefined) set((s) => ({ stageMarks: { ...s.stageMarks, [end]: position } }));
  },

  async start() {
    const plan = planOf(get());
    set({ progress: { taken: 0, total: totalShots(plan) } });
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

/** A number of a parameter: 2 is an integer, 3 a double; anything else, e.g. 0, not set. */
const numberOf = (value?: ParameterValue) =>
  value?.type === 3 ? value.double_value : value?.type === 2 ? value.integer_value : undefined;

/**
 * What rig.yaml sets for the stack, from the commander's parameters: the
 * ratios of the axes and the stage's angles. A new connection, e.g. after the
 * rig restarted, drops the stage's marks: the angles are rig.yaml's again.
 */
async function readConfig() {
  useStack.setState({ stageMarks: {} });
  try {
    const names = [`mm_per_turn.${RAIL}`, `deg_per_turn.${STAGE}`, 'focus_stack.stage_from', 'focus_stack.stage_to'];
    const { values } = await ros().callService<{ values: ParameterValue[] }>(
      '/stepit_server/get_parameters', 'rcl_interfaces/srv/GetParameters', { names });
    const [mm, deg, from, to] = values.map(numberOf);
    useStack.setState({
      mmPerTurn: mm || undefined,
      degPerTurn: deg || undefined,
      stageConfig: { from: from ?? DEFAULT_PLAN.stageFrom, to: to ?? DEFAULT_PLAN.stageTo },
    });
  } catch {
    // No commander yet: the next connection asks again, with the defaults meanwhile.
  }
}

/** How often the stage's position may change the page, in milliseconds. */
const STAGE_PERIOD = 100;

/**
 * Follows the rig's configuration of the stack, read again on every
 * connection, and where the stage is. Returns a function that stops it.
 */
export function followStack(): () => void {
  void readConfig();
  const unsubscribe = onConnected(() => void readConfig());

  let stopStage = () => {};
  let last = 0;
  const followStage = () => {
    stopStage();
    stopStage = ros().subscribe<JointState>('/joint_states', 'sensor_msgs/msg/JointState', (state) => {
      const now = Date.now();
      if (now - last < STAGE_PERIOD) return;
      const index = state.name.indexOf(STAGE);
      if (index < 0) return;
      last = now;
      useStack.setState({ stagePosition: state.position[index] });
    });
  };
  followStage();
  const unsubscribeStage = onConnected(followStage);

  return () => {
    unsubscribe();
    unsubscribeStage();
    stopStage();
  };
}
