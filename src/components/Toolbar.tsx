import { useEffect, useRef, useState } from 'react';
import { useCamera } from '../camera/store';
import { useCommander } from '../commander/store';
import { LIGHTS_JACK, useLights } from '../freezer/lights';
import { lightsOn } from '../freezer/outputs';
import { useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { download, useShot, type ShotPicture } from '../shot/store';
import { CameraSettings } from './CameraSettings';
import { CameraIcon, DownloadIcon, HandIcon, LightIcon, PlayIcon, ShutterIcon, StopIcon } from './icons';
import { Popover } from './Popover';

/**
 * The commands of the rig, above the live view: the live view itself, a shot,
 * the lights and manual drive, then the panels of the camera's settings and of
 * the pictures.
 */
export function Toolbar() {
  return (
    <div className="toolbar">
      <LiveViewButton />
      <ShotButton />
      <LightsSwitch />
      <ManualDriveButton />
      <span className="row-spacer" />
      <CameraPanel />
      <PicturesPanel />
    </div>
  );
}

function LiveViewButton() {
  const { streaming, setStreaming } = useCamera();
  return streaming ? (
    <button onClick={() => void setStreaming(false)} title="Stop the live view">
      <StopIcon />
      Live view
    </button>
  ) : (
    <button onClick={() => void setStreaming(true)} title="Start the live view">
      <PlayIcon />
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
 * The lights, on a jack of StepIt Freezer. The switch shows what the board
 * does: a shot ends with every output off, lights included.
 */
function LightsSwitch() {
  const { outputs, switching, setLights } = useLights();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const known = outputs !== undefined;
  const on = known && lightsOn(outputs);
  return (
    <button
      className={`switch${on ? ' on' : ''}`}
      role="switch"
      aria-checked={on}
      title={known ? `The lights, on OUT${LIGHTS_JACK} of StepIt Freezer` : 'Waiting for StepIt Freezer'}
      // A shot owns every output of the board: the Freezer refuses a change meanwhile.
      disabled={!connected || !known || busy || switching}
      onClick={() => void setLights(!on)}
    >
      <LightIcon />
      Lights
      <span className="switch-track"><span className="switch-thumb" /></span>
    </button>
  );
}

/**
 * Hands the robot to the sliders, through the objective ActivateTeleop. The
 * button stays, and shows when manual drive is on; pressing it again changes
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
      {enabled && <span className="state-chip">On</span>}
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

/** The pictures of this session, the latest first. The panel opens when a new one arrives. */
function PicturesPanel() {
  const pictures = useShot((s) => s.pictures);
  const [open, setOpen] = useState(false);
  const count = useRef(pictures.length);
  useEffect(() => {
    if (pictures.length > count.current) setOpen(true);
    count.current = pictures.length;
  }, [pictures.length]);
  const [latest, ...earlier] = pictures;

  return (
    <Popover
      title="The pictures of this session"
      open={open}
      onOpenChange={setOpen}
      align="right"
      button={<>Pictures{pictures.length > 0 && <span className="count-chip">{pictures.length}</span>}</>}
    >
      <h2>Pictures</h2>
      {!latest && <p className="muted">No picture yet. Take a shot: each picture appears here as the camera downloads it.</p>}
      {latest && <Picture picture={latest} large />}
      {earlier.length > 0 && (
        <ul className="pictures">
          {earlier.map((picture) => (
            <li key={picture.file}>
              <Picture picture={picture} />
            </li>
          ))}
        </ul>
      )}
    </Popover>
  );
}

function Picture({ picture, large = false }: { picture: ShotPicture; large?: boolean }) {
  const [error, setError] = useState<string>();
  const [saving, setSaving] = useState(false);
  const save = async () => {
    setSaving(true);
    setError(undefined);
    try {
      await download(picture);
    } catch (e) {
      setError(errorMessage(e));
    } finally {
      setSaving(false);
    }
  };

  return (
    <figure className={large ? 'shot large' : 'shot'}>
      {large && (picture.url ? (
        <a href={picture.url} target="_blank" rel="noreferrer">
          <img src={picture.url} alt={picture.name} />
        </a>
      ) : (
        <div className="shot-placeholder">
          {picture.error ?? (picture.size === undefined ? 'Loading…' : 'The browser cannot show this file')}
        </div>
      ))}
      <figcaption>
        <span className="shot-name">
          <strong>{picture.name}</strong>
          {picture.size !== undefined && <span className="muted"> {(picture.size / 1e6).toFixed(1)} MB</span>}
          {large && picture.preview && <span className="muted"> · its JPEG preview</span>}
        </span>
        <button onClick={() => void save()} disabled={saving} title={`Save ${picture.name} on this device`}>
          <DownloadIcon />
          {saving ? 'Saving…' : 'Download'}
        </button>
      </figcaption>
      {error && <p className="message error small">{error}</p>}
    </figure>
  );
}
