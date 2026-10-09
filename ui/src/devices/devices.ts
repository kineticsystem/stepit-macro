// The devices of the rig, and whether each one is connected, as its own node
// says on its status topic: the motors' controller, the camera, the Freezer
// board and the gamepad. Each topic is latched, and published again every second, so
// that a page that opens late gets the state at once, and one that stops
// hearing from a node knows it is gone.

import type { Rosbridge } from '../ros/rosbridge';

/** A device of the rig, as its icon in the top bar shows it. */
export type DeviceId = 'motors' | 'camera' | 'freezer' | 'gamepad';

/** What a status topic carries, whichever module publishes it. */
export interface StatusMessage {
  connected: boolean;
  /** The serial port, the camera's model, the joystick device, or "fake". */
  device: string;
  /** Why it is not connected; empty when it is. */
  message: string;
}

export interface Device {
  id: DeviceId;
  label: string;
  /** The status topic, given the camera node of the settings, e.g. /camera. */
  topic(cameraNode: string): string;
  type: string;
}

export const DEVICES: Device[] = [
  { id: 'camera', label: 'Camera', topic: (cameraNode) => `${cameraNode}/status`,
    type: 'stepit_camera_msgs/msg/CameraStatus' },
  { id: 'motors', label: 'Motors', topic: () => '/motors/status', type: 'stepit_motors_msgs/msg/MotorsStatus' },
  { id: 'freezer', label: 'Freezer', topic: () => '/freezer/status', type: 'freezer_msgs/msg/ControllerStatus' },
  { id: 'gamepad', label: 'Gamepad', topic: () => '/gamepad_teleop/status',
    type: 'stepit_teleop_msgs/msg/GamepadStatus' },
];

/**
 * How long a node may stay silent before it counts as gone, in milliseconds:
 * three of its messages missed.
 */
export const SILENCE_MS = 3500;

/** The last status heard from a device, and when, by the page's clock. */
export interface Heard {
  status: StatusMessage;
  at: number;
}

/**
 * - connected: the node talks to its device.
 * - disconnected: the node runs, and says it has lost its device.
 * - gone: the node said nothing for SILENCE_MS, e.g. it died.
 * - unknown: the page cannot tell, e.g. it is not connected to the rig, or it
 *   has just connected.
 */
export type DeviceState = 'connected' | 'disconnected' | 'gone' | 'unknown';

/**
 * The state of a device, from what the page last heard of it: since is when
 * the page connected to the rig, undefined while it is not.
 */
export function deviceState(heard: Heard | undefined, since: number | undefined, now: number): DeviceState {
  if (since === undefined) return 'unknown';
  const last = heard && heard.at >= since ? heard : undefined;
  if (!last) return now - since > SILENCE_MS ? 'gone' : 'unknown';
  if (now - last.at > SILENCE_MS) return 'gone';
  return last.status.connected ? 'connected' : 'disconnected';
}

/**
 * The text of a device's icon, e.g. "Motors: Lost the StepIt controller: ...":
 * online tells whether the page is connected to the rig.
 */
export function deviceText(device: Device, state: DeviceState, heard: Heard | undefined, online: boolean): string {
  switch (state) {
    case 'connected': {
      const name = heard?.status.device;
      return `${device.label}: connected${name ? ` (${name})` : ''}`;
    }
    case 'disconnected':
      return `${device.label}: ${heard?.status.message || 'not connected'}`;
    case 'gone':
      return `${device.label}: no news from its node, which may have stopped`;
    case 'unknown':
      return `${device.label}: ${online ? 'waiting for its node' : 'unknown, not connected to the rig'}`;
  }
}

/**
 * Calls onStatus with every status of every device, the latched one first.
 * Returns a function that stops it.
 */
export function subscribeStatuses(
  ros: Rosbridge, cameraNode: string, onStatus: (id: DeviceId, status: StatusMessage) => void,
): () => void {
  const stops = DEVICES.map((device) => ros.subscribe<StatusMessage>(device.topic(cameraNode), device.type,
    (message) => onStatus(device.id, message),
    { reliability: 'reliable', durability: 'transient_local', history: 'keep_last', depth: 1 }));
  return () => stops.forEach((stop) => stop());
}

/** The text of the connection's icon: whether the page reaches the rig's rosbridge at url. */
export function connectionText(status: 'connected' | 'connecting' | 'disconnected', url: string): string {
  switch (status) {
    case 'connected':
      return `Rig: connected to rosbridge at ${url}`;
    case 'connecting':
      return `Rig: connecting to rosbridge at ${url}`;
    case 'disconnected':
      return `Rig: not connected to rosbridge at ${url}, trying again every 2 s`;
  }
}
