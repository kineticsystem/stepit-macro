import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { createHold, HOLD_MS } from '../src/components/hold';

describe('a press that tells a tap from a hold', () => {
  const onHold = vi.fn();
  const onTap = vi.fn();
  const onChange = vi.fn();
  beforeEach(() => {
    vi.useFakeTimers();
    vi.clearAllMocks();
  });
  afterEach(() => vi.useRealTimers());

  it('is a tap when released early', () => {
    const hold = createHold({ onHold, onTap, onChange });
    hold.press();
    vi.advanceTimersByTime(HOLD_MS - 100);
    hold.release();
    expect(onTap).toHaveBeenCalledOnce();
    expect(onHold).not.toHaveBeenCalled();
    expect(onChange.mock.calls).toEqual([[true], [false]]);
  });

  it('is a hold once it lasted long enough, and nothing more when released', () => {
    const hold = createHold({ onHold, onTap });
    hold.press();
    vi.advanceTimersByTime(HOLD_MS);
    hold.release();
    expect(onHold).toHaveBeenCalledOnce();
    expect(onTap).not.toHaveBeenCalled();
  });

  // A thumb that slides off the button at the end of a drag.
  it('is nothing when abandoned', () => {
    const hold = createHold({ onHold, onTap });
    hold.press();
    vi.advanceTimersByTime(HOLD_MS - 100);
    hold.cancel();
    vi.advanceTimersByTime(HOLD_MS);
    hold.release();
    expect(onHold).not.toHaveBeenCalled();
    expect(onTap).not.toHaveBeenCalled();
  });

  // A key held down repeats its keydown.
  it('counts a repeated press once', () => {
    const hold = createHold({ onHold, onTap });
    hold.press();
    vi.advanceTimersByTime(300);
    hold.press();
    vi.advanceTimersByTime(HOLD_MS - 300);
    expect(onHold).toHaveBeenCalledOnce();
  });
});
