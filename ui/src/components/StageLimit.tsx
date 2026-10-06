import { useStack } from '../stack/store';

/**
 * An end of the stage's turn in the stack, at the end of its slider, as the
 * rail's marks are: shown, not set here, as the stack bar's Stage fields give
 * them. The slider turns the stage the positive way, clockwise, when pushed
 * up, so the larger angle shows above it and the smaller below, each on the
 * side the slider turns toward it.
 */
export function StageLimit({ side }: { side: 'top' | 'bottom' }) {
  const { stageFrom, stageTo } = useStack();
  const top = side === 'top';
  const angle = top ? Math.max(stageFrom, stageTo) : Math.min(stageFrom, stageTo);
  const which = angle === stageFrom && angle !== stageTo ? 'From' : angle === stageTo && angle !== stageFrom ? 'To' : 'From, to';
  return (
    <div className={`rail-mark ${side}`} title="The stack bar's Stage fields set it">
      <span className="stage-limit">{which}</span>
      <span className="mono small">{`${angle}°`}</span>
    </div>
  );
}
