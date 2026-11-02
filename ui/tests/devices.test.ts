import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import {
  connectionText, DEVICES, deviceState, deviceText, SILENCE_MS, subscribeStatuses, type DeviceId, type Heard, type StatusMessage,
} from '../src/devices/devices';
import { Rosbridge } from '../src/ros/rosbridge';
import { FakeSocket } from './fakeSocket';

const motors = DEVICES.find((d) => d.id === 'motors')!;
const heard = (connected: boolean, at: number, message = '', device = '/dev/ttyACM0'): Heard =>
  ({ status: { connected, device, message }, at });

describe('the state of a device', () => {
  const since = 1000;

  it('is unknown while the page is not connected to the rig', () => {
    expect(deviceState(heard(true, 2000), undefined, 2100)).toBe('unknown');
  });

  it('is what its node said last, while it keeps saying it', () => {
    expect(deviceState(heard(true, 2000), since, 2100)).toBe('connected');
    expect(deviceState(heard(false, 2000, 'Lost the StepIt controller'), since, 2100)).toBe('disconnected');
  });

  it('is a warning when the Freezer\'s controller has missed status replies', () => {
    const freezer = (missed: number): Heard => ({ status: { connected: true, device: '/dev/ttyACM0', message: '',
      missed_status: missed }, at: 2000 });
    expect(deviceState(freezer(0), since, 2100)).toBe('connected');
    expect(deviceState(freezer(3), since, 2100)).toBe('warning');
    expect(deviceState({ ...freezer(3), status: { ...freezer(3).status, connected: false } }, since, 2100))
      .toBe('disconnected');
  });

  it('is gone when its node has been silent too long', () => {
    expect(deviceState(heard(true, 2000), since, 2000 + SILENCE_MS + 1)).toBe('gone');
  });

  it('waits for a node at first, then counts it gone if it never speaks', () => {
    expect(deviceState(undefined, since, since + 100)).toBe('unknown');
    expect(deviceState(undefined, since, since + SILENCE_MS + 1)).toBe('gone');
  });

  it('ignores what was heard before the page connected again', () => {
    expect(deviceState(heard(true, 500), since, since + 100)).toBe('unknown');
  });
});

describe('the text of a device', () => {
  it('names the device it talks to, or tells why not', () => {
    expect(deviceText(motors, 'connected', heard(true, 0), true)).toBe('Motors: connected (/dev/ttyACM0)');
    const freezer = DEVICES.find((d) => d.id === 'freezer')!;
    const missed = (n: number): Heard => ({ status: { connected: true, device: 'x', message: '', missed_status: n }, at: 0 });
    expect(deviceText(freezer, 'warning', missed(1), true)).toMatch(/^Freezer: connected, but 1 status reply missed/);
    expect(deviceText(freezer, 'warning', missed(4), true)).toMatch(/4 status replies missed .* a healthy board misses none/);
    expect(deviceText(motors, 'disconnected', heard(false, 0, 'Lost the StepIt controller: IO Exception'), true))
      .toBe('Motors: Lost the StepIt controller: IO Exception');
    expect(deviceText(motors, 'gone', undefined, true)).toMatch(/no news from its node/);
    expect(deviceText(motors, 'unknown', undefined, false)).toMatch(/not connected to the rig/);
    expect(deviceText(motors, 'unknown', undefined, true)).toMatch(/waiting/);
  });
});

describe('the text of the connection', () => {
  it('names the rosbridge, and says the page tries again', () => {
    expect(connectionText('connected', 'ws://stepit:9090')).toBe('Rig: connected to rosbridge at ws://stepit:9090');
    expect(connectionText('connecting', 'ws://stepit:9090')).toMatch(/^Rig: connecting/);
    expect(connectionText('disconnected', 'ws://stepit:9090')).toMatch(/trying again every 2 s$/);
  });
});

describe('the status topics over rosbridge', () => {
  let ros: Rosbridge;
  let socket: FakeSocket;

  beforeEach(() => {
    FakeSocket.all = [];
    vi.stubGlobal('WebSocket', FakeSocket);
    ros = new Rosbridge('ws://rig:9090');
    socket = FakeSocket.last;
    socket.open();
  });
  afterEach(() => vi.unstubAllGlobals());

  it('follows the latched status of each device, the camera under the node of the settings', () => {
    const received: [DeviceId, StatusMessage][] = [];
    const stop = subscribeStatuses(ros, '/rig_camera', (id, status) => received.push([id, status]));

    const subscribed = socket.sent.filter((m) => m.op === 'subscribe');
    expect(subscribed.map((m) => [m.topic, m.type])).toEqual([
      ['/rig_camera/status', 'stepit_camera_msgs/msg/CameraStatus'],
      ['/motors/status', 'stepit_motors_msgs/msg/MotorsStatus'],
      ['/freezer/status', 'freezer_msgs/msg/ControllerStatus'],
      ['/gamepad_teleop/status', 'stepit_teleop_msgs/msg/GamepadStatus'],
    ]);
    expect(subscribed.every((m) => m.qos?.durability === 'transient_local')).toBe(true);

    const status = { connected: false, device: '/dev/ttyUSB0', message: 'No Freezer controller.' };
    socket.receive({ op: 'publish', topic: '/freezer/status', msg: status });
    expect(received).toEqual([['freezer', status]]);

    stop();
    expect(socket.sent.filter((m) => m.op === 'unsubscribe')).toHaveLength(4);
  });
});
