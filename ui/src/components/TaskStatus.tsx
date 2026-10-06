import { useCommander } from '../commander/store';

/**
 * The task that runs, in the top bar, and why the last one of this page
 * failed. Nothing when no task runs: the Stop button of the toolbar is then
 * off.
 */
export function TaskStatus() {
  const { busy, running, objective, failure } = useCommander();

  // The commander names what runs, whichever page or device sent it; a tree
  // that threw may leave its name behind, so only while something runs.
  let text: string | undefined;
  const name = running || objective;
  if (busy || running) text = name ? `Running ${name}` : 'A task is running';

  return (
    <div className="task">
      {text && <span className="task-state active">{text}</span>}
      {failure && !running && <span className="task-failure" title={failure}>{failure}</span>}
    </div>
  );
}
