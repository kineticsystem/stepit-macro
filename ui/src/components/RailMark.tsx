import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { useStack, type End } from '../stack/store';

/**
 * A mark of the stack, at the end of the rail's slider that it marks: above
 * it the camera's position away from the subject, where its front is sharp,
 * the objective MarkNear; below it the camera's position close to the
 * subject, where its back is sharp, MarkFar. The slider moves the camera away
 * when pushed up, so each button sits on the side of the rail it records.
 *
 * It reads Marked once its end is. It shows no position: one counted from
 * wherever the motor started would mean nothing; the depth between the two
 * marks shows in the stack's bar. Pressing it again marks the end again.
 */
export function RailMark({ end }: { end: End }) {
  const { marking, mark } = useStack();
  const position = useStack((s) => s[end]);
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const away = end === 'near';
  return (
    <div className={`rail-mark ${away ? 'top' : 'bottom'}`}>
      <button
        disabled={!connected || busy || marking !== undefined}
        title={
          away
            ? 'The camera, away from the subject: its front is sharp. Mark this end of the stack.'
            : 'The camera, close to the subject: its back is sharp. Mark this end of the stack.'
        }
        onClick={() => void mark(end)}
      >
        {marking === end ? 'Marking…' : position === undefined ? 'Mark' : 'Marked'}
      </button>
    </div>
  );
}
