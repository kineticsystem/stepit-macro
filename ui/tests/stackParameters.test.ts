import { describe, expect, it } from 'vitest';
import { countParameter, numberOf, stackOf } from '../src/stack/parameters';

describe('the stack the rig keeps, in the commander parameters', () => {
  it('reads the marks, the counts and the stage turn', () => {
    expect(stackOf([
      { name: 'state.turn', value: { type: 8, double_array_value: [17] } },
      { name: 'state.near', value: { type: 8, double_array_value: [210.96] } },
      { name: 'state.far', value: { type: 8, double_array_value: [111.15] } },
      { name: 'state.shots', value: { type: 8, double_array_value: [40] } },
      { name: 'state.angles', value: { type: 8, double_array_value: [3] } },
    ])).toEqual({ turn: 17, near: 210.96, far: 111.15, shots: 40, angles: 3 });
  });

  // A parameter that is not set, e.g. a mark never made, comes back with type 0.
  it('leaves out what is not set, and every other parameter', () => {
    expect(stackOf([
      { name: 'state.near', value: { type: 0 } },
      { name: 'focus_stack.turn', value: { type: 3, double_value: 17 } },
    ])).toEqual({});
  });

  it('takes the first number of a list', () => {
    expect(numberOf({ type: 8, double_array_value: [12.5, 1] })).toBe(12.5);
    expect(numberOf({ type: 8, double_array_value: [] })).toBeUndefined();
  });

  it('saves a count as the list of one number the rig keeps', () => {
    expect(countParameter('shots', 40)).toEqual({ name: 'state.shots', value: { type: 8, double_array_value: [40] } });
  });
});
