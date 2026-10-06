import { describe, expect, it } from 'vitest';
import { payloadOf, planProblem, railStep, railText, totalShots } from '../src/stack/plan';

const plan = { shots: 10, stageFrom: -0.25, stageTo: 0.25, angles: 35 };

describe('the plan of a focus stack', () => {
  it('takes a picture at every shot of every angle', () => {
    expect(totalShots(plan)).toBe(350);
  });

  // The page speaks turns of the motors; FocusStack takes radians.
  it('asks FocusStack for the stage in radians', () => {
    expect(payloadOf(plan)).toBe('{shots: 10, stage_from: -1.570796, stage_to: 1.570796, angles: 35}');
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
});
