import { useEffect, useState } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { planProblem } from '../stack/plan';
import { planOf, useStack } from '../stack/store';
import { LayersIcon } from './icons';

/**
 * A focus stack, under the live view: Stack, which shoots it, and the
 * counts; while it runs, a bar of the pictures taken, in the space left. The ends themselves are at
 * the ends of the sliders, see RailMark and StageMark: for the rail, we drive the camera away from the subject until its front is
 * sharp and mark above the slider, then close to it until its back is sharp
 * and mark below. Stack runs FocusStack, which takes the robot, and shows how
 * many pictures came; Stop, in the toolbar, ends it.
 */
export function StackBar() {
  const stack = useStack();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const idle = connected && !busy;
  const plan = planOf(stack);
  const problem = planProblem(plan);

  return (
    <div className="stack-bar">
      <button
        className="primary"
        disabled={!idle || problem !== undefined || stack.progress !== undefined}
        title={problem ?? 'Shoot the stack: the robot moves'}
        onClick={() => void stack.start()}
      >
        <LayersIcon />
        Stack
      </button>
      <NumberField label="Shots" value={stack.shots} integer onChange={(shots) => stack.setPlan({ shots })} />
      <NumberField label="Angles" value={stack.angles} integer onChange={(angles) => stack.setPlan({ angles })} />
      {stack.progress && (
        <progress
          className="stack-progress"
          max={stack.progress.total}
          value={stack.progress.taken}
          aria-label="Pictures of the stack"
          title={`${stack.progress.taken} of ${stack.progress.total} pictures`}
        />
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
