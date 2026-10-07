import { describe, expect, it } from 'vitest';
import { angleStep, DEFAULT_PLAN, payloadOf, planProblem, stageRange, totalShots } from '../src/stack/plan';

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

  it('turns the stage from -17 to 17 degrees by default, one stack every degree', () => {
    expect(DEFAULT_PLAN).toMatchObject({ stageFrom: -17, stageTo: 17, angles: 35 });
  });

  // One turn either way, as the page sets it: -17 to 17.
  it('turns the stage as far either way', () => {
    expect(stageRange(17)).toEqual({ stageFrom: -17, stageTo: 17 });
    expect(planProblem({ ...plan, ...stageRange(-1), near: 1, far: 2 })).toMatch(/negative/);
  });

  it('spaces the angles evenly over the turn', () => {
    expect(angleStep(17, 35)).toBeCloseTo(1);
    expect(angleStep(17, 1)).toBeUndefined();
  });
});
