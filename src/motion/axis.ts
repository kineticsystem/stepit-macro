// A slider as an axis of a gamepad: -1 at its left end, 0 at its centre, 1 at
// its right end. The rig turns the axis into a velocity, up to the motors' limit
// at the ends (ui_teleop in rig.yaml).

/** Around the centre, the axis is 0: a finger never rests exactly there. */
export const DEAD_ZONE = 0.05;

/** The motors' limit, which the ends of a slider command: 3 turns/s. */
export const MAX_TURNS_PER_SECOND = 3;

/** The axis at a horizontal position over a slider, which starts at left and is width wide. */
export function axisAt(x: number, left: number, width: number): number {
  if (width <= 0) return 0;
  const value = Math.max(-1, Math.min(1, (x - left - width / 2) / (width / 2)));
  return Math.abs(value) < DEAD_ZONE ? 0 : value;
}

/** The speed an axis asks for, in turns per second of the motor. */
export function turnsPerSecond(axis: number): number {
  return axis * MAX_TURNS_PER_SECOND;
}
