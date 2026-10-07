import { useEffect, useMemo, useState, type KeyboardEvent } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { useStack, type End } from '../stack/store';
import { createHold } from './hold';
import { CheckIcon } from './icons';

/** How long the hint to hold shows after a tap on an unmarked end, in milliseconds. */
const HINT_MS = 1500;

/**
 * A mark of the stack, at the end of the rail's slider that it marks: above
 * it the camera's position away from the subject, where its front is sharp,
 * the objective MarkNear; below it the camera's position close to the
 * subject, where its back is sharp, MarkFar. The slider moves the camera away
 * when pushed up, so each button sits on the side of the rail it records.
 *
 * **Hold to mark, tap to go.** The button is where a thumb lands at the end of
 * a drag of the slider, so marking takes a press of HOLD_MS, which fills the
 * button as it lasts: a brush marks nothing. Once both ends are marked, a tap
 * moves the rail back to one, MoveRailToMark, to check the focus there, then
 * manual drive again; before, a tap says what to do. A new mark flashes, even on an end marked already. Green with a check
 * while marked; a rig start forgets the marks, and every page shows Mark again.
 */
export function RailMark({ end }: { end: End }) {
  const { marking, flashed } = useStack();
  const marked = useStack((s) => s[end] !== undefined);
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const [holding, setHolding] = useState(false);
  const [hint, setHint] = useState<string>();
  const disabled = !connected || busy || marking !== undefined;
  const away = end === 'near';

  useEffect(() => {
    if (!hint) return;
    const timer = setTimeout(() => setHint(undefined), HINT_MS);
    return () => clearTimeout(timer);
  }, [hint]);

  const hold = useMemo(
    () =>
      createHold({
        onHold: () => void useStack.getState().mark(end),
        // Going back approaches the mark from the near end toward the far one,
        // as a stack does: it needs both.
        onTap: () => {
          const { near, far, goToMark } = useStack.getState();
          if (near !== undefined && far !== undefined) void goToMark(end);
          else setHint(useStack.getState()[end] === undefined ? 'Hold to mark' : 'Mark both ends');
        },
        onChange: setHolding,
      }),
    [end],
  );

  const key = (e: KeyboardEvent, down: boolean) => {
    if (e.key !== ' ' && e.key !== 'Enter') return;
    e.preventDefault();
    if (down) hold.press();
    else hold.release();
  };

  let label = marked ? 'Marked' : 'Mark';
  if (marking === end) label = 'Marking…';
  else if (hint) label = hint;

  const side = away
    ? 'The camera, away from the subject: its front is sharp.'
    : 'The camera, close to the subject: its back is sharp.';
  return (
    <div className={`rail-mark ${away ? 'top' : 'bottom'}`}>
      <button
        className={['hold', holding && 'holding', marked && 'engaged', flashed === end && 'flash'].filter(Boolean).join(' ')}
        disabled={disabled}
        title={`${side} Hold to mark this end of the stack; once both ends are marked, tap to go back to it.`}
        onPointerDown={(e) => e.button === 0 && hold.press()}
        onPointerUp={() => hold.release()}
        onPointerCancel={() => hold.cancel()}
        // A touch stays on the button it began on: leaving it is a move out of its bounds.
        onPointerMove={(e) => {
          const r = e.currentTarget.getBoundingClientRect();
          if (e.clientX < r.left || e.clientX > r.right || e.clientY < r.top || e.clientY > r.bottom) hold.cancel();
        }}
        onPointerLeave={() => hold.cancel()}
        onKeyDown={(e) => key(e, true)}
        onKeyUp={(e) => key(e, false)}
        onContextMenu={(e) => e.preventDefault()}
      >
        {marked && marking !== end && !hint && <CheckIcon />}
        {label}
      </button>
    </div>
  );
}
