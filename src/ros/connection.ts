// The connection to the rig's rosbridge, shared by the whole page. It is the
// commander's, which knows the interfaces of every module: they are installed
// next to each other in the rig's container.

import { create } from 'zustand';
import { rosbridgeUrl, useSettings } from '../settings';
import { Rosbridge, type Status } from './rosbridge';

let current: Rosbridge | undefined;

/** The status of the connection, for React. */
const useStatusStore = create<{ status: Status }>(() => ({ status: 'disconnected' }));

/** The connection to the rig, opened on first use, and again when the settings point elsewhere. */
export function ros(): Rosbridge {
  const url = rosbridgeUrl(useSettings.getState());
  if (!current || current.url !== url) {
    current?.close();
    const opened = new Rosbridge(url);
    current = opened;
    useStatusStore.setState({ status: opened.getStatus() });
    opened.onStatus((status) => current === opened && useStatusStore.setState({ status }));
  }
  return current;
}

export function status(): Status {
  return useStatusStore.getState().status;
}

/** The status of the connection, as a React hook. */
export function useStatus(): Status {
  return useStatusStore((s) => s.status);
}

/** Calls the listener whenever the connection is lost. Returns a function that stops it. */
export function onDisconnected(listener: () => void): () => void {
  return useStatusStore.subscribe((s, previous) => {
    if (s.status !== 'connected' && previous.status === 'connected') listener();
  });
}

/** Calls the listener whenever the connection is established. Returns a function that stops it. */
export function onConnected(listener: () => void): () => void {
  return useStatusStore.subscribe((s, previous) => {
    if (s.status === 'connected' && previous.status !== 'connected') listener();
  });
}

// Another rosbridge in the settings: connect to it.
useSettings.subscribe((settings, previous) => {
  if (rosbridgeUrl(settings) !== rosbridgeUrl(previous)) ros();
});
