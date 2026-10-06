import { describe, expect, it } from 'vitest';
import { DEFAULT_PLAN, payloadOf, planProblem, totalShots } from '../src/stack/plan';

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
});
