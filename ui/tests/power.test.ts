import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { POWER_HOLD_MS, POWER_OFF, powerOff, powerProblem, TRIGGER } from '../src/power/power';
import { createHold } from '../src/components/hold';
import { Rosbridge } from '../src/ros/rosbridge';
import { FakeSocket } from './fakeSocket';

describe('switching the rig off', () => {
  let ros: Rosbridge;
  let socket: FakeSocket;

  beforeEach(() => {
    FakeSocket.all = [];
    vi.stubGlobal('WebSocket', FakeSocket);
    ros = new Rosbridge('ws://rig:9090');
    socket = FakeSocket.last;
    socket.open();
  });
  afterEach(() => {
    vi.unstubAllGlobals();
    vi.useRealTimers();
  });

  it("asks power_off's service, which says the rig is switching off", async () => {
    const asked = powerOff(ros);
    expect(socket.lastSent('call_service')).toMatchObject({ service: POWER_OFF, type: TRIGGER });
    socket.respond({ success: true, message: 'Switching off' });
    await expect(asked).resolves.toEqual({ ok: true, message: 'Switching off' });
  });

  it('passes on why power_off refused', async () => {
    const asked = powerOff(ros);
    socket.respond({ success: false, message: 'FocusStack is running: stop it first' });
    await expect(asked).resolves.toEqual({ ok: false, message: 'FocusStack is running: stop it first' });
  });

  it('is off while a stack runs, disconnected, or switching off, and says why', () => {
    expect(powerProblem(true, '', false)).toBeUndefined();
    expect(powerProblem(true, 'TakeShot', false)).toBeUndefined();
    expect(powerProblem(true, 'FocusStack', false)).toBe('FocusStack is running: stop it first');
    expect(powerProblem(true, 'Stack', false)).toBe('Stack is running: stop it first');
    expect(powerProblem(false, '', false)).toBe('Not connected to the rig');
    expect(powerProblem(true, '', true)).toBe('The rig is switching off');
  });

  it('switches off after a hold of 3 seconds, not sooner', () => {
    vi.useFakeTimers();
    const onHold = vi.fn();
    const onTap = vi.fn();
    const hold = createHold({ onHold, onTap }, POWER_HOLD_MS);
    hold.press();
    vi.advanceTimersByTime(2900);
    hold.release();
    expect(onHold).not.toHaveBeenCalled();
    expect(onTap).toHaveBeenCalledOnce();

    hold.press();
    vi.advanceTimersByTime(3000);
    expect(onHold).toHaveBeenCalledOnce();
  });
});
