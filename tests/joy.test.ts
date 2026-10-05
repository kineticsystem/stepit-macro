import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { JOY_TOPIC, JOY_TYPE, JoyPublisher, PERIOD } from '../src/motion/joy';

describe('the sliders sent as a gamepad', () => {
  let sent: number[][];
  let joy: JoyPublisher;

  beforeEach(() => {
    vi.useFakeTimers();
    sent = [];
    joy = new JoyPublisher((topic, type, message) => {
      expect([topic, type]).toEqual([JOY_TOPIC, JOY_TYPE]);
      sent.push((message as { axes: number[] }).axes);
    }, 2);
  });
  afterEach(() => vi.useRealTimers());

  it('sends a change at once', () => {
    joy.set(0, 0.5);
    expect(sent).toEqual([[0.5, 0]]);
    joy.set(1, -1);
    expect(sent).toEqual([[0.5, 0], [0.5, -1]]);
  });

  // ui_teleop stops the joints when the axes stop coming for 0.5 s.
  it('repeats the axes 20 times a second while a slider is held still', () => {
    joy.set(0, 0.5);
    vi.advanceTimersByTime(PERIOD * 10);
    expect(sent).toHaveLength(11);
    expect(sent.every((axes) => axes[0] === 0.5)).toBe(true);
  });

  it('sends the axes once at rest when let go, and then no more', () => {
    joy.set(0, 0.5);
    joy.set(0, 0);
    expect(sent).toEqual([[0.5, 0], [0, 0]]);
    vi.advanceTimersByTime(PERIOD * 10);
    expect(sent).toHaveLength(2);
  });

  it('keeps repeating while another slider is held', () => {
    joy.set(0, 0.5);
    joy.set(1, 1);
    joy.set(0, 0);
    vi.advanceTimersByTime(PERIOD * 2);
    expect(sent.slice(-2)).toEqual([[0, 1], [0, 1]]);
  });

  it('lets every slider go at once, and does nothing when none is held', () => {
    joy.release();
    expect(sent).toEqual([]);
    joy.set(0, 1);
    joy.set(1, -1);
    joy.release();
    expect(sent.at(-1)).toEqual([0, 0]);
    vi.advanceTimersByTime(PERIOD * 10);
    expect(sent.at(-1)).toEqual([0, 0]);
    expect(sent).toHaveLength(3);
  });
});
