import { describe, expect, it } from 'vitest';
import { axisAt, DEAD_ZONE } from '../src/motion/axis';

describe('a vertical slider as an axis', () => {
  // A slider from y = 100, its top, to y = 300: its centre is at 200.
  it('is 0 at the centre, 1 at the top and -1 at the bottom, and stays there beyond them', () => {
    expect(axisAt(200, 100, 200)).toBe(0);
    expect(axisAt(100, 100, 200)).toBe(1);
    expect(axisAt(300, 100, 200)).toBe(-1);
    expect(axisAt(150, 100, 200)).toBe(0.5);
    expect(axisAt(0, 100, 200)).toBe(1);
    expect(axisAt(999, 100, 200)).toBe(-1);
  });

  it('is 0 near the centre, where a thumb never rests exactly', () => {
    expect(axisAt(200 - (DEAD_ZONE * 100) / 2, 100, 200)).toBe(0);
    expect(axisAt(200 - DEAD_ZONE * 100 * 2, 100, 200)).toBeGreaterThan(0);
  });

  it('is 0 on a slider that has no height, e.g. before it is laid out', () => {
    expect(axisAt(10, 0, 0)).toBe(0);
  });
});
