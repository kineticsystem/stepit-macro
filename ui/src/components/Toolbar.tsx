import { useCamera } from '../camera/store';
import { useCommander } from '../commander/store';
import { LIGHTS_JACK, useLights } from '../freezer/lights';
import { lightsOn } from '../freezer/outputs';
import { useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { useSettings } from '../settings';
import { useShot } from '../shot/store';
import { HandIcon, LayersIcon, LightIcon, ShutterIcon, StopIcon, VideoIcon } from './icons';

/**
 * The commands of the rig, above the live view: a shot first, then the live
 * view, the lights, manual drive and the stack bar, and Stop at the other end.
 * The camera's settings are in the settings menu, under the gear.
 * The last photo shows in place of the live view. Live view, Lights and
 * Manual drive are buttons that turn green while they are on.
 */
export function Toolbar() {
  return (
    <div className="toolbar">
      <ShotButton />
      <LiveViewButton />
      <LightsButton />
      <ManualDriveButton />
      <StackButton />
      <span className="row-spacer" />
      <StopButton />
    </div>
  );
}

/** Shows or hides the stack bar, under the toolbar: highlighted while shown. */
function StackButton() {
  const { showStack, update } = useSettings();
  return (
    <button
      className={showStack ? 'active' : ''}
      aria-pressed={showStack}
      title={showStack ? 'Hide the focus stack' : 'Show the focus stack: its marks, its plan, Start'}
      onClick={() => update({ showStack: !showStack })}
    >
      <LayersIcon />
      Stack
    </button>
  );
}

/** The live view: a button, green while the stream runs. */
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
 * Manual drive, on and off: hands the robot to the sliders, through the
 * objective ActivateTeleop, and back to the trajectory controller, through
 * ActivateController. Green while it is on.
 */
function ManualDriveButton() {
  const { enabled, enable, disable } = useMotion();
  const running = useCommander((s) => s.running);
  const connected = useStatus() === 'connected';
  return (
    <button
      className={enabled ? 'engaged' : ''}
      aria-pressed={enabled}
      title={enabled ? 'The sliders drive the joints: press to end manual drive' : 'Hand the robot to the sliders'}
      disabled={!connected || running === 'ActivateTeleop' || running === 'ActivateController'}
      onClick={() => void (enabled ? disable() : enable())}
    >
      <HandIcon />
      Manual drive
    </button>
  );
}

/**
 * Stops every task, whoever started it, and lets the sliders go. On, red,
 * while a task runs, and while the page does not know yet whether one does,
 * e.g. right after it connected; off otherwise. Always in the same place.
 */
function StopButton() {
  const { busy, known, running, stop } = useCommander();
  const release = useMotion((s) => s.release);
  const connected = useStatus() === 'connected';
  const on = connected && (busy || running !== undefined || !known);
  return (
    <button
      className="stop"
      disabled={!on}
      title={on ? 'Stop every task' : 'No task runs'}
      onClick={() => {
        release();
        void stop();
      }}
    >
      <StopIcon />
      Stop
    </button>
  );
}
