import { useCommander } from '../commander/store';

/**
 * The task that runs, in the top bar, and why the last one of this page
 * failed. Nothing when no task runs: the Stop button of the toolbar is then
 * off.
 */
export function TaskStatus() {
  const { busy, running, failure } = useCommander();

  let text: string | undefined;
  if (running) text = `Running ${running}`;
  else if (busy) text = 'A task is running';

  return (
    <div className="task">
      {text && <span className="task-state active">{text}</span>}
      {failure && !running && <span className="task-failure" title={failure}>{failure}</span>}
    </div>
  );
}
