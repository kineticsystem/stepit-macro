import { useEffect, useState } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { planProblem, railStep, totalShots } from '../stack/plan';
import { useStack, type End } from '../stack/store';

/**
 * A focus stack, under the toolbar, beside the live view that the marks are
 * set by: the two ends of the rail, the plan, and Start.
 *
 * We drive the rail, with the gamepad or the sliders, until the closest part
 * of the subject that must be sharp is in focus, and press Set near; then the
 * farthest, and Set far. Marking moves nothing and leaves manual drive on.
 * Start runs FocusStack, which takes the robot, and shows how many pictures
 * came; Stop, in the toolbar, ends it.
 */
export function StackBar() {
  const stack = useStack();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const idle = connected && !busy;
  const problem = planProblem(stack);
  const step = stack.near !== undefined && stack.far !== undefined ? railStep(stack.near, stack.far, stack.shots) : undefined;

  return (
    <div className="stack-bar">
      <MarkButton end="near" />
      <MarkButton end="far" />
      <span className="stack-divider" />
      <NumberField label="Shots" value={stack.shots} integer onChange={(shots) => stack.setPlan({ shots })} />
      <NumberField label="Stage" value={stack.stageFrom} onChange={(stageFrom) => stack.setPlan({ stageFrom })} />
      <NumberField label="to" unit="turns" value={stack.stageTo} onChange={(stageTo) => stack.setPlan({ stageTo })} />
      <NumberField label="Angles" value={stack.angles} integer onChange={(angles) => stack.setPlan({ angles })} />
      <span className="stack-summary muted small">
        {totalShots(stack)} pictures
        {step !== undefined && ` · ${step.toFixed(3)} turns apart`}
      </span>
      <span className="row-spacer" />
      {stack.progress ? (
        <span className="stack-progress">
          Picture {stack.progress.taken} of {stack.progress.total}
        </span>
      ) : (
        <button
          className="primary"
          disabled={!idle || problem !== undefined}
          title={problem ?? 'Shoot the stack: the robot moves'}
          onClick={() => void stack.start()}
        >
          Start stack
        </button>
      )}
    </div>
  );
}

/** Set near or Set far, with where the rail was when this page marked it. */
function MarkButton({ end }: { end: End }) {
  const { marking, mark } = useStack();
  const position = useStack((s) => s[end]);
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const name = end === 'near' ? 'near' : 'far';
  return (
    <span className="stack-mark">
      <button
        disabled={!connected || busy || marking !== undefined}
        title={`The rail is at the ${name} end of the subject: remember it`}
        onClick={() => void mark(end)}
      >
        {marking === end ? 'Marking…' : `Set ${name}`}
      </button>
      <span className="mono small">{position === undefined ? '—' : `${position.toFixed(3)} turns`}</span>
    </span>
  );
}

/** A number, applied when it is valid: an empty or partial entry leaves the plan as it was. */
function NumberField(props: { label: string; unit?: string; value: number; integer?: boolean; onChange(value: number): void }) {
  const [text, setText] = useState(String(props.value));
  useEffect(() => setText(String(props.value)), [props.value]);
  const apply = (raw: string) => {
    setText(raw);
    const value = Number(raw);
    if (raw.trim() !== '' && Number.isFinite(value) && (!props.integer || Number.isInteger(value))) props.onChange(value);
  };
  return (
    <label className="stack-field">
      <span className="setting-label">{props.label}</span>
      <input
        type="number"
        inputMode={props.integer ? 'numeric' : 'decimal'}
        step={props.integer ? 1 : 0.05}
        min={props.integer ? 1 : undefined}
        value={text}
        onChange={(e) => apply(e.target.value)}
      />
      {props.unit && <span className="muted small">{props.unit}</span>}
    </label>
  );
}
