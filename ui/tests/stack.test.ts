import { describe, expect, it } from 'vitest';
import { DEFAULT_PLAN, payloadOf, planProblem, railStep, railText, stageAngle, totalShots } from '../src/stack/plan';

const plan = { shots: 10, stageFrom: -17, stageTo: 17, angles: 35 };

describe('the plan of a focus stack', () => {
  it('takes a picture at every shot of every angle', () => {
    expect(totalShots(plan)).toBe(350);
  });

  // FocusStack converts the degrees itself, with the stage's ratio in rig.yaml.
  it('asks FocusStack for the stage in degrees', () => {
    expect(payloadOf(plan)).toBe('{shots: 10, stage_from: -17, stage_to: 17, angles: 35}');
  });

  it('needs both marks', () => {
    expect(planProblem({ ...plan, near: 1 })).toMatch(/Mark the near and the far end/);
    expect(planProblem({ ...plan, near: 1, far: 2 })).toBeUndefined();
  });

  it('refuses a number of shots or angles that is not whole', () => {
    expect(planProblem({ ...plan, near: 1, far: 2, shots: 2.5 })).toMatch(/Shots/);
    expect(planProblem({ ...plan, near: 1, far: 2, angles: 0 })).toMatch(/Angles/);
  });

  it('spaces the shots evenly between the marks, whichever is larger', () => {
    expect(railStep(1, 2, 5)).toBeCloseTo(0.25);
    expect(railStep(2, 1, 5)).toBeCloseTo(0.25);
    expect(railStep(1, 2, 1)).toBeUndefined();
  });

  // 1.592 mm per turn, measured on the rig.
  it('shows the rail in millimetres once its ratio is known, in turns before', () => {
    expect(railText(10, 1.592)).toBe('15.92 mm');
    expect(railText(10)).toBe('10.000 turns');
  });

  it('turns the stage from -17 to 17 degrees by default, one stack every degree', () => {
    expect(DEFAULT_PLAN).toMatchObject({ stageFrom: -17, stageTo: 17, angles: 35 });
  });

  // An end of the stage, in degrees from where the stage is now, 4.5 degrees per motor turn.
  it('shows a configured end of the stage as it is', () => {
    expect(stageAngle(-17)).toBe(-17);
  });

  it('shows a marked end of the stage from where the stage is now', () => {
    const turn = 2 * Math.PI;
    // Marked 2 motor turns, 9 degrees, past where the stage is now.
    expect(stageAngle(-17, 3 * turn, 1 * turn, 4.5)).toBeCloseTo(9);
    expect(stageAngle(-17, 1 * turn, 3 * turn, 4.5)).toBeCloseTo(-9);
  });

  it('cannot show a mark without the ratio or the position', () => {
    expect(stageAngle(-17, 1, undefined, 4.5)).toBeUndefined();
    expect(stageAngle(-17, 1, 0, undefined)).toBeUndefined();
  });
});
