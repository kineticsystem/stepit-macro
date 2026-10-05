import { useCommander } from '../commander/store';
import { useMotion } from '../motion/store';
import { useStatus } from '../ros/connection';
import { StopIcon } from './icons';

/**
 * Whether a task runs, and the button that stops every task and the sliders:
 * always there, whatever runs, and whoever started it.
 */
export function TaskStatus() {
  const { busy, running, failure, stop } = useCommander();
  const release = useMotion((s) => s.release);
  const connected = useStatus() === 'connected';

  let text = 'Idle';
  if (running) text = `Running ${running}`;
  else if (busy) text = 'A task is running';

  return (
    <div className="task">
      <span className={`task-state${busy || running ? ' active' : ''}`}>{text}</span>
      {failure && !running && <span className="task-failure" title={failure}>{failure}</span>}
      <button
        className="stop"
        disabled={!connected}
        onClick={() => {
          release();
          void stop();
        }}
      >
        <StopIcon />
        Stop
      </button>
    </div>
  );
}
