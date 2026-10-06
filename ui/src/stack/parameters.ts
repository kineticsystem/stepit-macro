// The stack as the rig keeps it, in the commander's parameters: rig.yaml's
// configuration, focus_stack.*, and what the objectives and the pages saved
// in the rig's state file, state.*: the marks of the rail and the counts.
// Apart from the store, so that it can be tested without a browser.

/** A value of rcl_interfaces/msg/Parameter: the types the stack uses. */
export interface ParameterValue {
  type: number;
  integer_value?: number;
  double_value?: number;
  double_array_value?: number[];
}

export interface Parameter {
  name: string;
  value: ParameterValue;
}

/** ParameterType of rcl_interfaces. */
const INTEGER = 2;
const DOUBLE = 3;
const DOUBLE_ARRAY = 8;

/** What the stack's store takes from the parameters. */
export interface StackParameters {
  shots?: number;
  angles?: number;
  /** The marks of the rail, in radians of its motor: where the rig saved them. */
  near?: number;
  far?: number;
  stageConfig?: { from?: number; to?: number };
}

/** The names the page reads: the stage's angles, the marks, the counts. */
export const STACK_PARAMETERS = [
  'focus_stack.stage_from', 'focus_stack.stage_to', 'state.near', 'state.far', 'state.shots', 'state.angles',
];

/** A number of a parameter, the first of a list; undefined for a parameter that is not set. */
export function numberOf(value?: ParameterValue): number | undefined {
  switch (value?.type) {
    case INTEGER: return value.integer_value;
    case DOUBLE: return value.double_value;
    case DOUBLE_ARRAY: return value.double_array_value?.[0];
    default: return undefined;
  }
}

/** What the given parameters say of the stack; the others are left out. */
export function stackOf(parameters: Parameter[]): StackParameters {
  const stack: StackParameters = {};
  for (const { name, value } of parameters) {
    const number = numberOf(value);
    if (number === undefined) continue;
    switch (name) {
      case 'focus_stack.stage_from': stack.stageConfig = { ...stack.stageConfig, from: number }; break;
      case 'focus_stack.stage_to': stack.stageConfig = { ...stack.stageConfig, to: number }; break;
      case 'state.near': stack.near = number; break;
      case 'state.far': stack.far = number; break;
      case 'state.shots': stack.shots = number; break;
      case 'state.angles': stack.angles = number; break;
    }
  }
  return stack;
}

/** The parameter that saves a count, e.g. state.shots, as SetParameters takes it. */
export function countParameter(key: 'shots' | 'angles', value: number): Parameter {
  return { name: `state.${key}`, value: { type: DOUBLE_ARRAY, double_array_value: [value] } };
}
