// The plan of a focus stack, apart from the store, so that it can be tested
// without a browser: what FocusStack is asked, and whether it can be.

/** A turn of a motor, in radians: the page shows turns, the objectives take radians. */
export const TURN = 2 * Math.PI;

export interface StackPlan {
  /** Shots from the near mark to the far one, both included. */
  shots: number;
  /** The first and the last angle of the stage, in degrees, from where it is. */
  stageFrom: number;
  stageTo: number;
  /** Angles from the first to the last, both included: 1 for a single stack. */
  angles: number;
}

/**
 * By default, 10 shots at 35 angles from -17 to 17 degrees: one every degree.
 * The rig's own come from the commander's parameters, which rig.yaml and the
 * rig's state file set; these are for before the page has read them.
 */
export const DEFAULT_PLAN: StackPlan = { shots: 10, stageFrom: -17, stageTo: 17, angles: 35 };

/** An end of the stage's turn: from, below its slider, and to, above. */
export type StageEnd = 'from' | 'to';

/** How many pictures a plan takes. */
export const totalShots = (plan: StackPlan) => plan.shots * plan.angles;

/** Why a plan cannot run, if it cannot. */
export function planProblem(plan: StackPlan & { near?: number; far?: number }): string | undefined {
  if (plan.near === undefined || plan.far === undefined) return 'Mark the near and the far end first';
  if (!Number.isInteger(plan.shots) || plan.shots < 1) return 'Shots must be a whole number, at least 1';
  if (!Number.isInteger(plan.angles) || plan.angles < 1) return 'Angles must be a whole number, at least 1';
  if (!Number.isFinite(plan.stageFrom) || !Number.isFinite(plan.stageTo)) return 'The stage needs two angles';
  return undefined;
}

/** The payload of FocusStack: the rig turns the degrees into radians of the stage's motor. */
export function payloadOf(plan: StackPlan): string {
  return `{shots: ${plan.shots}, stage_from: ${plan.stageFrom}, stage_to: ${plan.stageTo}, angles: ${plan.angles}}`;
}
