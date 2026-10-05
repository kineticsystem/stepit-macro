import { useStatus } from '../ros/connection';
import { rosbridgeUrl, useSettings } from '../settings';

const LABELS = { connected: 'Connected', connecting: 'Connecting…', disconnected: 'Disconnected' };

/** Whether the page reaches the rig's rosbridge. */
export function ConnectionBadge() {
  const status = useStatus();
  const url = useSettings((s) => rosbridgeUrl(s));
  return (
    <span className={`badge badge-${status}`} title={`rosbridge at ${url}`}>
      <span className="dot" />
      {LABELS[status]}
    </span>
  );
}
