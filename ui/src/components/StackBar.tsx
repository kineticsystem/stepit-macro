import { useEffect, useState } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { planProblem, railStep, railText, totalShots } from '../stack/plan';
import { useStack } from '../stack/store';

/**
 * A focus stack, under the toolbar: the plan, the depth between the two
 * marks, and Start. The marks themselves are at the ends of the rail's slider,
 * see RailMark: we drive the camera away from the subject until its front is
 * sharp and mark above the slider, then close to it until its back is sharp
 * and mark below. Start runs FocusStack, which takes the robot, and shows how
 * many pictures came; Stop, in the toolbar, ends it.
 */
export function StackBar() {
  const stack = useStack();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const idle = connected && !busy;
  const problem = planProblem(stack);
  const marked = stack.near !== undefined && stack.far !== undefined;
  const depth = marked ? Math.abs(stack.far! - stack.near!) : undefined;
  const step = marked ? railStep(stack.near!, stack.far!, stack.shots) : undefined;

  return (
    <div className="stack-bar">
      <NumberField label="Shots" value={stack.shots} integer onChange={(shots) => stack.setPlan({ shots })} />
      <NumberField label="Stage" value={stack.stageFrom} onChange={(stageFrom) => stack.setPlan({ stageFrom })} />
      <NumberField label="to" unit="°" value={stack.stageTo} onChange={(stageTo) => stack.setPlan({ stageTo })} />
      <NumberField label="Angles" value={stack.angles} integer onChange={(angles) => stack.setPlan({ angles })} />
      <span className="stack-summary muted small">
        {depth !== undefined ? `${railText(depth, stack.mmPerTurn)} deep · ` : 'Mark both ends on the rail · '}
        {totalShots(stack)} pictures
        {step !== undefined && ` · ${railText(step, stack.mmPerTurn)} apart`}
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
        step={1}
        min={props.integer ? 1 : undefined}
        value={text}
        onChange={(e) => apply(e.target.value)}
      />
      {props.unit && <span className="muted small">{props.unit}</span>}
    </label>
  );
}
