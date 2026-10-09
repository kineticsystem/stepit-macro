import { useEffect, useRef, useState, type ReactNode } from 'react';
import { connectionText, DEVICES, deviceState, deviceText, type DeviceId } from '../devices/devices';
import { useDevices } from '../devices/store';
import { useStatus } from '../ros/connection';
import { rosbridgeUrl, useSettings } from '../settings';
import { CameraIcon, GamepadIcon, LinkIcon, MotorIcon, TriggerIcon } from './icons';

const ICONS: Record<DeviceId, ReactNode> = {
  camera: <CameraIcon />, motors: <MotorIcon />, freezer: <TriggerIcon />, gamepad: <GamepadIcon />,
};

/**
 * The connection to the rig's rosbridge, first: green when connected, orange
 * while connecting, red and struck through when disconnected. Then an icon per
 * device of the rig, camera, motors, Freezer and gamepad: green when its node
 * talks to it, red and struck through when it does not or its node is silent,
 * grey when the page cannot tell. A tap tells why, as a tablet has no tooltip.
 */
export function DeviceStatus() {
  const { heard, since, now } = useDevices();
  const status = useStatus();
  const online = status === 'connected';
  const url = useSettings((s) => rosbridgeUrl(s));
  const [open, setOpen] = useState<DeviceId | 'rig'>();
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const close = (e: PointerEvent) => {
      if (!ref.current?.contains(e.target as Node)) setOpen(undefined);
    };
    document.addEventListener('pointerdown', close);
    return () => document.removeEventListener('pointerdown', close);
  }, [open]);

  const devices = DEVICES.map((device) => {
    const state = deviceState(heard[device.id], since, now);
    return { device, state, text: deviceText(device, state, heard[device.id], online) };
  });
  const rig = connectionText(status, url);
  const shown = open === 'rig' ? rig : devices.find((d) => d.device.id === open)?.text;

  return (
    <div className="devices" ref={ref}>
      <button className={`icon-button device connection-${status}`} title={rig} aria-label={rig}
        onClick={() => setOpen(open === 'rig' ? undefined : 'rig')}>
        <LinkIcon />
      </button>
      {devices.map(({ device, state, text }) => (
        <button key={device.id} className={`icon-button device device-${state}`} title={text} aria-label={text}
          onClick={() => setOpen(open === device.id ? undefined : device.id)}>
          {ICONS[device.id]}
        </button>
      ))}
      {shown && <div className="device-popover" role="status">{shown}</div>}
    </div>
  );
}
