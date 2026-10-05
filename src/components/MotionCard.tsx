import { useCommander } from '../commander/store';
import { SLIDERS, useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { HandIcon } from './icons';
import { Slider } from './Slider';

/**
 * The rotary stage and the rail, driven by hand as with the gamepad. The
 * sliders work once the robot is handed to the user, by the objective
 * ActivateTeleop, which stops whatever moves the robot.
 *
 * The button stays where it is, and shows whether manual drive is on: a button
 * that vanished once pressed would look like a fault, and move the sliders.
 * Pressing it again is harmless: ActivateTeleop then changes nothing.
 */
export function MotionCard() {
  const { enabled, axes, enable, setAxis } = useMotion();
  const running = useCommander((s) => s.running);
  const connected = useStatus() === 'connected';

  return (
    <section className="card motion">
      <div className="card-header">
        <h2>Motion</h2>
        <button
          className={enabled ? 'engaged' : 'primary'}
          aria-pressed={enabled}
          disabled={!connected || running === 'ActivateTeleop'}
          onClick={() => void enable()}
        >
          <HandIcon />
          Manual drive
          {enabled && <span className="state-chip">On</span>}
        </button>
      </div>
      <p className="muted small">
        {enabled
          ? 'The sliders drive the joints. Any task, e.g. a move, takes the robot back.'
          : 'The sliders drive the joints once the robot is handed to you.'}
      </p>
      {SLIDERS.map((slider) => (
        <Slider
          key={slider.axis}
          label={slider.label}
          value={axes[slider.axis]}
          disabled={!enabled}
          onChange={(value) => setAxis(slider.axis, value)}
        />
      ))}
    </section>
  );
}
