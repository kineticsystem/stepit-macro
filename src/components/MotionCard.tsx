import { useCommander } from '../commander/store';
import { SLIDERS, useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { HandIcon } from './icons';
import { Slider } from './Slider';

/**
 * The rotary stage and the rail, driven by hand as with the gamepad. The
 * sliders work once the robot is handed to the user, by the objective
 * ActivateTeleop, which stops whatever moves the robot.
 */
export function MotionCard() {
  const { enabled, axes, enable, setAxis } = useMotion();
  const running = useCommander((s) => s.running);
  const connected = useStatus() === 'connected';

  return (
    <section className="card motion">
      <div className="card-header">
        <h2>Motion</h2>
        {!enabled && (
          <button className="primary" disabled={!connected || running === 'ActivateTeleop'} onClick={() => void enable()}>
            <HandIcon />
            Drive by hand
          </button>
        )}
      </div>
      {!enabled && <p className="muted small">The sliders drive the joints once the robot is handed to you.</p>}
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
