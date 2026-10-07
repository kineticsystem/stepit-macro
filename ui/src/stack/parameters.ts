// The stack as the rig keeps it, in the commander's parameters: what the
// objectives and the pages saved in the rig's state file, state.*: the marks
// of the rail, the counts and the stage's turn, rig.yaml's focus_stack.* until
// a page sets them.
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
  /** How far the stage turns either way, in degrees from where it is when the stack starts. */
  turn?: number;
  /** The marks of the rail, in radians of its motor: where the rig saved them. */
  near?: number;
  far?: number;
}

/** The names the page reads: the marks, the counts, the stage's turn. */
export const STACK_PARAMETERS = ['state.near', 'state.far', 'state.shots', 'state.angles', 'state.turn'];

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
      case 'state.near': stack.near = number; break;
      case 'state.far': stack.far = number; break;
      case 'state.shots': stack.shots = number; break;
      case 'state.angles': stack.angles = number; break;
      case 'state.turn': stack.turn = number; break;
    }
  }
  return stack;
}

/** What a page sets of a stack: the counts and the stage's turn, kept on the rig. */
export type PlanKey = 'shots' | 'angles' | 'turn';

/** The parameter that saves a value of the plan, e.g. state.shots, as SetParameters takes it. */
export function countParameter(key: PlanKey, value: number): Parameter {
  return { name: `state.${key}`, value: { type: DOUBLE_ARRAY, double_array_value: [value] } };
}
