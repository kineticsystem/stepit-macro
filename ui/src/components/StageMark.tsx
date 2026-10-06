import { useStatus } from '../ros/connection';
import type { StageEnd } from '../stack/plan';
import { stageAngleOf, useStack } from '../stack/store';

/**
 * An end of the stage's turn in the stack, at the end of its slider, as the
 * rail's marks are: "to" above, on the side the slider turns the stage, the
 * positive way, clockwise; "from" below. It shows the end's angle from where
 * the stage is now: rig.yaml's, focus_stack.stage_from or stage_to, until Mark
 * sets it to where the stage is. A mark stays on this page alone, and goes
 * when the page connects to the rig again, e.g. after a restart.
 */
export function StageMark({ end }: { end: StageEnd }) {
  const state = useStack();
  const connected = useStatus() === 'connected';
  const angle = stageAngleOf(state, end);
  const marked = state.stageMarks[end] !== undefined;
  return (
    <div className={`rail-mark ${end === 'to' ? 'top' : 'bottom'}`}>
      <button
        disabled={!connected || state.stagePosition === undefined}
        title={`The stage turns to here at the ${end === 'to' ? 'end' : 'start'} of the stack. Until the rig restarts.`}
        onClick={() => state.markStage(end)}
      >
        Mark
      </button>
      <span className="mono small" title={marked ? 'Marked here, from where the stage is now' : 'From rig.yaml'}>
        {angle === undefined ? '—' : `${end === 'to' ? 'To' : 'From'} ${angle.toFixed(1)}°`}
        {marked ? ' •' : ''}
      </span>
    </div>
  );
}
