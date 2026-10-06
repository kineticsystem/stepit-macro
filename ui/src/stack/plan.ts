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

export const DEFAULT_PLAN: StackPlan = { shots: 10, stageFrom: 0, stageTo: 0, angles: 1 };

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

/** The distance between two shots of the rail, in turns of its motor, if there are two. */
export function railStep(near: number, far: number, shots: number): number | undefined {
  return shots > 1 ? Math.abs(far - near) / (shots - 1) : undefined;
}

/**
 * A position or a distance of the rail, in millimetres when its ratio is
 * known, mm_per_turn.joint2 of the commander, in turns of its motor otherwise.
 */
export function railText(turns: number, mmPerTurn?: number): string {
  return mmPerTurn ? `${(turns * mmPerTurn).toFixed(2)} mm` : `${turns.toFixed(3)} turns`;
}
