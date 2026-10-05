import { useCommander } from '../commander/store';
import { LIGHTS_JACK, useLights } from '../freezer/lights';
import { lightsOn } from '../freezer/outputs';
import { useStatus } from '../ros/connection';
import { LightIcon } from './icons';

/**
 * The lights, on a jack of StepIt Freezer, switched by hand. The switch shows
 * what the board does: a shot ends with every output off, lights included.
 */
export function LightsCard() {
  const { outputs, switching, error, setLights } = useLights();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const known = outputs !== undefined;
  const on = known && lightsOn(outputs);

  return (
    <section className="card">
      <h2>Lights</h2>
      <button
        className={`switch${on ? ' on' : ''}`}
        role="switch"
        aria-checked={on}
        // A shot owns every output of the board: the Freezer refuses a change meanwhile.
        disabled={!connected || !known || busy || switching}
        onClick={() => void setLights(!on)}
      >
        <LightIcon />
        <span className="switch-label">{on ? 'On' : 'Off'}</span>
        <span className="switch-track"><span className="switch-thumb" /></span>
      </button>
      {!known && connected && <p className="message muted">Waiting for StepIt Freezer…</p>}
      {error && <p className="message error">{error}</p>}
      <p className="muted small">OUT{LIGHTS_JACK} of StepIt Freezer.</p>
    </section>
  );
}
