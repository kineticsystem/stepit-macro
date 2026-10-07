import { useEffect, useState } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { angleStep, planProblem } from '../stack/plan';
import { planOf, useStack } from '../stack/store';
import { LayersIcon } from './icons';

/**
 * A focus stack, under the live view: Stack, which shoots it, the shots of
 * the rail, how far the stage turns either way, and the angles, with the angle
 * between two stacks; while it runs, a bar of the pictures taken, in the space left. The ends themselves are at
 * the ends of the sliders, see RailMark and StageMark: for the rail, we drive the camera away from the subject until its front is
 * sharp and mark above the slider, then close to it until its back is sharp
 * and mark below. Stack runs FocusStack, which takes the robot, and shows how
 * many pictures came; Stop, in the toolbar, ends it. With one angle, the
 * stage stays where it is, and Turn is off. Why Stack is off is in the top
 * bar, see TaskStatus.
 */
export function StackBar() {
  const stack = useStack();
  const busy = useCommander((s) => s.busy);
  // The rig's progress, while it runs a stack, whichever page started it.
  const stacking = useCommander((s) => s.busy && s.objective === 'FocusStack');
  const progress = stacking ? stack.progress : undefined;
  const connected = useStatus() === 'connected';
  const idle = connected && !busy;
  const plan = planOf(stack);
  const step = angleStep(stack.turn, stack.angles);
  const problem = planProblem(plan);

  return (
    <div className="stack-bar">
      <button
        className="primary main-action"
        disabled={!idle || problem !== undefined}
        title={problem ?? 'Shoot the stack: the robot moves'}
        onClick={() => void stack.start()}
      >
        <LayersIcon />
        Stack
      </button>
      <NumberField label="Shots" value={stack.shots} integer onChange={(shots) => void stack.setPlan({ shots })} />
      <NumberField label="Turn ±" unit="°" value={stack.turn} min={0} disabled={stack.angles === 1}
        title={stack.angles === 1 ? 'One angle: the stage stays where it is' : undefined}
        onChange={(turn) => void stack.setPlan({ turn })} />
      <NumberField label="Angles" value={stack.angles} integer onChange={(angles) => void stack.setPlan({ angles })} />
      {step !== undefined && <span className="muted stack-step">{`${step.toFixed(1)}° apart`}</span>}
      {stack.angles === 1 && <span className="muted stack-step">one angle</span>}
      {stack.error && <span className="error small">{stack.error}</span>}
      {progress && (
        <progress
          className="stack-progress"
          max={progress.total}
          value={progress.taken}
          aria-label="Pictures of the stack"
          title={`${progress.taken} of ${progress.total} pictures`}
        />
      )}
    </div>
  );
}

/** A number, applied when it is valid: an empty or partial entry leaves the plan as it was. */
function NumberField(props: {
  label: string; unit?: string; value: number; integer?: boolean; min?: number; disabled?: boolean; title?: string;
  onChange(value: number): void;
}) {
  const [text, setText] = useState(String(props.value));
  useEffect(() => setText(String(props.value)), [props.value]);
  const apply = (raw: string) => {
    setText(raw);
    const value = Number(raw);
    const valid = raw.trim() !== '' && Number.isFinite(value) && (!props.integer || Number.isInteger(value));
    if (valid && (props.min === undefined || value >= props.min)) props.onChange(value);
  };
  return (
    <label className="stack-field" title={props.title}>
      <span className="setting-label">{props.label}</span>
      <input
        type="number"
        inputMode={props.integer ? 'numeric' : 'decimal'}
        step={1}
        min={props.min ?? (props.integer ? 1 : undefined)}
        value={text}
        disabled={props.disabled}
        onChange={(e) => apply(e.target.value)}
      />
      {props.unit && <span className="muted">{props.unit}</span>}
    </label>
  );
}
