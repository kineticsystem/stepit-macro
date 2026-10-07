// The rig's commander, StepIt Commander, over rosbridge: it runs the tasks of
// the rig, the objectives, one at a time. A new objective replaces the one
// running, which stops what it was doing.
//
//   /commander/execute_objective                     the action: tree and payload
//   /commander/execute_objective/_action/status      the goals, whoever sent them
//   /commander/execute_objective/_action/cancel_goal  stops them, all at once
//
// Only tasks go through the commander. Configuring the rig, e.g. the settings
// of the camera or the lights, goes straight to the drivers.

import type { Rosbridge } from '../ros/rosbridge';

export const ACTION = '/commander/execute_objective';
export const EXECUTE_TREE = 'btcpp_ros2_interfaces/action/ExecuteTree';

/** GoalStatus of action_msgs. */
const SUCCEEDED = 4;
const ACTIVE = new Set([1, 2, 3]); // accepted, executing, canceling
/** NodeStatus of btcpp_ros2_interfaces: how the tree itself ended. */
const TREE_SUCCESS = 2;

export interface RunResult {
  /** The goal succeeded and the tree returned SUCCESS. */
  ok: boolean;
  message: string;
}

interface ExecuteTreeResult {
  node_status?: { status?: number };
  return_message?: string;
}

interface GoalStatusArray {
  status_list: { status: number }[];
}

/** Runs an objective, e.g. TakeShot, and returns how it ended. */
export async function runObjective(ros: Rosbridge, objective: string, payload = ''): Promise<RunResult> {
  const goal = ros.sendActionGoal<ExecuteTreeResult>(ACTION, EXECUTE_TREE, { target_tree: objective, payload });
  const { status, values } = await goal.result;
  const ok = status === SUCCEEDED && values?.node_status?.status === TREE_SUCCESS;
  return { ok, message: values?.return_message || describe(status) };
}

/** Stops every objective, whoever sent it. */
export async function cancelAll(ros: Rosbridge): Promise<void> {
  // A goal of zeros, at time zero, cancels all of them.
  await ros.callService(`${ACTION}/_action/cancel_goal`, 'action_msgs/srv/CancelGoal', {});
}

/**
 * Calls the listener with whether an objective runs, sent by this page or by
 * anyone else, e.g. the gamepad. Returns a function that stops it.
 */
export function followObjectives(ros: Rosbridge, listener: (running: boolean) => void): () => void {
  // The QoS of an action's status topic: the latest status reaches a late subscriber.
  return ros.subscribe<GoalStatusArray>(`${ACTION}/_action/status`, 'action_msgs/msg/GoalStatusArray',
    (message) => listener(message.status_list.some((goal) => ACTIVE.has(goal.status))),
    { reliability: 'reliable', durability: 'transient_local', history: 'keep_last', depth: 1 });
}

/** The topic where the commander publishes the objective running, latched: "" when none. */
const OBJECTIVE_TOPIC = '/stepit_server/objective';

/**
 * Calls the listener with the name of the objective running, whoever sent
 * it, or "" when none runs: at once, from the latched topic, and on every
 * change.
 */
export function followObjective(ros: Rosbridge, listener: (objective: string) => void): () => void {
  return ros.subscribe<{ data: string }>(OBJECTIVE_TOPIC, 'std_msgs/msg/String', (message) => listener(message.data),
    { reliability: 'reliable', durability: 'transient_local', history: 'keep_last', depth: 1 });
}

/**
 * Whether an objective runs, from both sources: the status of the action, which
 * a commander that just started has not published yet, since it publishes it
 * only for a goal, and the latched objective, which it publishes, empty, as
 * soon as it starts. Either is enough to know. The status, which tells every
 * goal, wins once it came: the objective keeps the name of a tree that threw
 * until the next one.
 * @param status Whether a goal is active, or undefined until the status came.
 * @param objective The objective running, "" for none, or undefined until it came.
 */
export function objectiveRuns(status: boolean | undefined, objective: string | undefined): { busy: boolean; known: boolean } {
  return { busy: status ?? !!objective, known: status !== undefined || objective !== undefined };
}

function describe(status: number): string {
  return { 4: 'Succeeded', 5: 'Stopped', 6: 'Failed' }[status] ?? `Ended with status ${status}`;
}
