// What the page hears of the devices of the rig, for the icons of the top bar.

import { create } from 'zustand';
import { onConnected, onDisconnected, ros, status } from '../ros/connection';
import { useSettings } from '../settings';
import { subscribeStatuses, type DeviceId, type Heard } from './devices';

/** How often the icons look at the clock, to see a node gone silent, in milliseconds. */
const TICK_MS = 500;

interface DevicesState {
  /** The last status of each device, since the page opened. */
  heard: Partial<Record<DeviceId, Heard>>;
  /** When the page connected to the rig; undefined while it is not. */
  since?: number;
  /** The page's clock, as of the last tick. */
  now: number;
}

export const useDevices = create<DevicesState>(() => ({ heard: {}, now: Date.now() }));

/**
 * Follows the status topic of every device while the page is open, with the
 * camera node the settings name. Returns a function that stops it.
 */
export function followDevices(): () => void {
  let stopFollowing = () => {};
  const follow = () => {
    stopFollowing();
    stopFollowing = subscribeStatuses(ros(), useSettings.getState().cameraNode, (id, message) =>
      useDevices.setState((s) => ({ heard: { ...s.heard, [id]: { status: message, at: Date.now() } } })));
  };
  follow();
  if (status() === 'connected') useDevices.setState({ since: Date.now() });
  // Again on every connection, as the other stores do: the settings may name
  // another rosbridge, a new Rosbridge without the subscriptions.
  const unsubscribeConnected = onConnected(() => {
    useDevices.setState({ since: Date.now(), now: Date.now() });
    follow();
  });
  const unsubscribeLost = onDisconnected(() => useDevices.setState({ since: undefined }));
  const unsubscribeSettings = useSettings.subscribe((settings, previous) => {
    if (settings.cameraNode !== previous.cameraNode) follow();
  });
  const timer = setInterval(() => useDevices.setState({ now: Date.now() }), TICK_MS);
  return () => {
    stopFollowing();
    unsubscribeConnected();
    unsubscribeLost();
    unsubscribeSettings();
    clearInterval(timer);
  };
}
