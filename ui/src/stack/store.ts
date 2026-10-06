// A focus stack: the ends of the rail and of the stage, the counts, and the
// run. The rig keeps all of it, never this browser, so that every page shows
// the same: the commander's parameters hold rig.yaml's configuration,
// focus_stack.*, and what was saved in the rig's state file, state.*.
//
// The rail's ends are marked by the rig, the objectives MarkNear and MarkFar,
// which save them in the state file, where FocusStack reads them. The counts
// of shots and angles a page sets go there too, through the parameters. The
// stage's ends are rig.yaml's angles, in degrees from where the stage is when
// the stack starts. The page reads them all on every connection, and follows
// them on /parameter_events.

import { create } from 'zustand';
import { camera } from '../camera/store';
import { useCommander } from '../commander/store';
import { onConnected, ros } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import type { RunResult } from '../commander/commander';
import { countParameter, STACK_PARAMETERS, stackOf, type Parameter, type ParameterValue } from './parameters';
import { DEFAULT_PLAN, payloadOf, totalShots, type StackPlan } from './plan';

/** The commander, whose parameters hold the stack. */
const COMMANDER = '/stepit_server';

export type End = 'near' | 'far';

interface StackState {
  shots: number;
  angles: number;
  /** The marks of the rail, where the rig saved them; undefined until marked. */
  near?: number;
  far?: number;
  /** The stage's angles of rig.yaml, in degrees from where it is when the stack starts. */
  stageConfig: { from: number; to: number };
  /** The rail end being marked. */
  marking?: End;
  /** Why the rig refused a count, if it did. */
  error?: string;
  /** While FocusStack runs from this page: the pictures taken so far, of how many. */
  progress?: { taken: number; total: number };

  setPlan(change: Partial<Pick<StackState, 'shots' | 'angles'>>): Promise<void>;
  mark(end: End): Promise<RunResult>;
  start(): Promise<RunResult>;
}

/** The plan FocusStack is asked for. */
export function planOf(state: StackState): StackPlan & { near?: number; far?: number } {
  return {
    shots: state.shots,
    angles: state.angles,
    stageFrom: state.stageConfig.from,
    stageTo: state.stageConfig.to,
    near: state.near,
    far: state.far,
  };
}

/** Takes what the parameters say of the stack. */
function apply(parameters: Parameter[]) {
  const { stageConfig, ...rest } = stackOf(parameters);
  useStack.setState((s) => ({ ...rest, ...(stageConfig && { stageConfig: { ...s.stageConfig, ...stageConfig } }) }));
}

export const useStack = create<StackState>((set, get) => ({
  shots: DEFAULT_PLAN.shots,
  angles: DEFAULT_PLAN.angles,
  stageConfig: { from: DEFAULT_PLAN.stageFrom, to: DEFAULT_PLAN.stageTo },

  async setPlan(change) {
    set({ ...change, error: undefined });
    const parameters = Object.entries(change).map(([key, value]) => countParameter(key as 'shots' | 'angles', value));
    try {
      const { results } = await ros().callService<{ results: { successful: boolean; reason: string }[] }>(
        `${COMMANDER}/set_parameters`, 'rcl_interfaces/srv/SetParameters', { parameters });
      const refused = results.find((r) => !r.successful);
      if (refused) set({ error: refused.reason });
    } catch (e) {
      set({ error: errorMessage(e) });
    }
  },

  async mark(end) {
    set({ marking: end });
    try {
      const result = await useCommander.getState().run(end === 'near' ? 'MarkNear' : 'MarkFar');
      // The rig saved it: /parameter_events says so, and so does reading it again.
      if (result.ok) await readStack();
      return result;
    } finally {
      set({ marking: undefined });
    }
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

/** Reads the stack from the commander's parameters. */
async function readStack() {
  try {
    const { values } = await ros().callService<{ values: ParameterValue[] }>(
      `${COMMANDER}/get_parameters`, 'rcl_interfaces/srv/GetParameters', { names: STACK_PARAMETERS });
    apply(STACK_PARAMETERS.map((name, i) => ({ name, value: values[i] })));
  } catch {
    // No commander yet: the next connection asks again, with the defaults meanwhile.
  }
}

interface ParameterEvent {
  node: string;
  new_parameters: Parameter[];
  changed_parameters: Parameter[];
}

/**
 * Follows the stack the rig keeps: reads it on every connection, and takes
 * every change on /parameter_events, e.g. a mark set from another page.
 * Returns a function that stops it.
 */
export function followStack(): () => void {
  void readStack();
  const unsubscribe = onConnected(() => void readStack());
  const stopEvents = ros().subscribe<ParameterEvent>('/parameter_events', 'rcl_interfaces/msg/ParameterEvent', (event) => {
    if (event.node === COMMANDER) apply([...event.new_parameters, ...event.changed_parameters]);
  });
  return () => {
    unsubscribe();
    stopEvents();
  };
}
