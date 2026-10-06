import type { StageEnd as End } from '../stack/plan';
import { useStack } from '../stack/store';

/**
 * An end of the stage's turn in the stack, at the end of its slider, laid out
 * as the rail's marks but only shown: rig.yaml's focus_stack.stage_to above,
 * on the side the slider turns the stage, the positive way, clockwise, and
 * stage_from below, in degrees from where the stage is when the stack starts.
 */
export function StageEnd({ end }: { end: End }) {
  const angle = useStack((s) => s.stageConfig[end]);
  return (
    <div className={`rail-mark ${end === 'to' ? 'top' : 'bottom'}`} title="Set in rig.yaml, focus_stack">
      <span className="stage-end">{`${end === 'to' ? 'To' : 'From'} ${angle}°`}</span>
    </div>
  );
}
