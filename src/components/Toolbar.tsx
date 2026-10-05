import { useState } from 'react';
import { useCamera } from '../camera/store';
import { useCommander } from '../commander/store';
import { LIGHTS_JACK, useLights } from '../freezer/lights';
import { lightsOn } from '../freezer/outputs';
import { useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { useShot } from '../shot/store';
import { CameraSettings } from './CameraSettings';
import { CameraIcon, HandIcon, LightIcon, ShutterIcon, VideoIcon } from './icons';
import { Popover } from './Popover';

/**
 * The commands of the rig, above the live view: a shot first, then the live
 * view, the lights and manual drive, then the panel of the camera's settings.
 * The last photo shows in place of the live view. Live view, Lights and
 * Manual drive are buttons that light up while they are on: teal for the live
 * view, green for the other two.
 */
export function Toolbar() {
  return (
    <div className="toolbar">
      <ShotButton />
      <LiveViewButton />
      <LightsButton />
      <ManualDriveButton />
      <span className="row-spacer" />
      <CameraPanel />
    </div>
  );
}

/** The live view: a button, lit while the stream runs. */
function LiveViewButton() {
  const { streaming, setStreaming } = useCamera();
  return (
    <button
      className={`live${streaming ? ' on' : ''}`}
      aria-pressed={streaming}
      title={streaming ? 'Stop the live view' : 'Start the live view'}
      onClick={() => void setStreaming(!streaming)}
    >
      <VideoIcon />
      Live view
    </button>
  );
}

/** A shot, fired by StepIt Freezer through the objective TakeShot. */
function ShotButton() {
  const { state, takeShot } = useShot();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const shooting = state === 'shooting' || state === 'waiting';
  return (
    <button className="primary" disabled={!connected || busy || shooting} onClick={() => void takeShot()}>
      <ShutterIcon />
      {shooting ? 'Shooting…' : 'Take a shot'}
    </button>
  );
}

/**
 * The lights, on a jack of StepIt Freezer: a button, green while the lights
 * are on. It shows what the board does: a shot ends with every output off, lights
 * included.
 */
function LightsButton() {
  const { outputs, switching, setLights } = useLights();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const known = outputs !== undefined;
  const on = known && lightsOn(outputs);
  return (
    <button
      className={`lights${on ? ' on' : ''}`}
      aria-pressed={on}
      title={known ? `The lights, on OUT${LIGHTS_JACK} of StepIt Freezer` : 'Waiting for StepIt Freezer'}
      // A shot owns every output of the board: the Freezer refuses a change meanwhile.
      disabled={!connected || !known || busy || switching}
      onClick={() => void setLights(!on)}
    >
      <LightIcon />
      Lights
    </button>
  );
}

/**
 * Hands the robot to the sliders, through the objective ActivateTeleop. The
 * button stays, green while manual drive is on; pressing it again changes
 * nothing.
 */
function ManualDriveButton() {
  const { enabled, enable } = useMotion();
  const running = useCommander((s) => s.running);
  const connected = useStatus() === 'connected';
  return (
    <button
      className={enabled ? 'engaged' : ''}
      aria-pressed={enabled}
      title={enabled ? 'The sliders drive the joints. Any task, e.g. a move, takes the robot back.' : 'Hand the robot to the sliders'}
      disabled={!connected || running === 'ActivateTeleop'}
      onClick={() => void enable()}
    >
      <HandIcon />
      Manual drive
    </button>
  );
}

function CameraPanel() {
  const [open, setOpen] = useState(false);
  const error = useCamera((s) => s.error);
  return (
    <Popover
      title="The camera's settings"
      open={open}
      onOpenChange={setOpen}
      align="right"
      button={<><CameraIcon />Camera{error && <span className="dot-warning" title={error} />}</>}
    >
      <CameraSettings />
    </Popover>
  );
}
