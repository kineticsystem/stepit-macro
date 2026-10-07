import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { planProblem } from '../stack/plan';
import { planOf, useStack } from '../stack/store';

/**
 * The task that runs, in the top bar, and why the last one of this page
 * failed. While nothing runs, why Stack is off, if it is, e.g. no mark yet:
 * a tablet shows no tooltip. Nothing else when no task runs: the Stop button
 * of the toolbar is then off.
 */
export function TaskStatus() {
  const { busy, running, objective, failure } = useCommander();
  const stack = useStack();
  const connected = useStatus() === 'connected';

  // The commander names what runs, whichever page or device sent it; a tree
  // that threw may leave its name behind, so only while something runs.
  let text: string | undefined;
  const name = running || objective;
  if (busy || running) text = name ? `Running ${name}` : 'A task is running';

  return (
    <div className="task">
      {text && <span className="task-state active">{text}</span>}
      {failure && !running && <span className="task-failure" title={failure}>{failure}</span>}
      {connected && !text && !failure && <StackHint problem={planProblem(planOf(stack))} />}
    </div>
  );
}

function StackHint({ problem }: { problem?: string }) {
  return problem ? <span className="task-state" title={problem}>Stack: {problem}</span> : null;
}

