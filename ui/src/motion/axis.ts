// A slider as an axis of a gamepad: -1 at its bottom end, 0 at its centre, 1 at
// its top end, as a gamepad's stick pushed up. The rig turns the axis into a
// velocity, up to the motors' limit at the ends (ui_teleop in rig.yaml).

/** Around the centre, the axis is 0: a finger never rests exactly there. */
export const DEAD_ZONE = 0.05;

/** The axis at a vertical position over a slider, which starts at top and is height high: 1 at the top. */
export function axisAt(y: number, top: number, height: number): number {
  if (height <= 0) return 0;
  const value = Math.max(-1, Math.min(1, (top + height / 2 - y) / (height / 2)));
  return Math.abs(value) < DEAD_ZONE ? 0 : value;
}
