import { describe, expect, it } from 'vitest';
import { jackBits, lightsOn, withLights } from '../src/freezer/outputs';

describe('the lights on a jack of StepIt Freezer', () => {
  it('are both lines of the jack: OUT1 bits 0 and 1, OUT8 bits 14 and 15', () => {
    expect(jackBits(1)).toBe(0b11);
    expect(jackBits(8)).toBe(0xc000);
  });

  it('switch on and off without touching the other outputs', () => {
    expect(withLights(0, true)).toBe(3);
    expect(withLights(0xc000, true)).toBe(0xc003);
    expect(withLights(0xc003, false)).toBe(0xc000);
    expect(withLights(0xffff, false)).toBe(0xfffc);
  });

  it('are on only when both lines are closed', () => {
    expect(lightsOn(3)).toBe(true);
    expect(lightsOn(1)).toBe(false);
    expect(lightsOn(0xc000)).toBe(false);
  });
});
