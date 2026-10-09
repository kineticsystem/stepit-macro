import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import {
  ACTION, cancelAll, EXECUTE_TREE, followObjective, objectiveRuns, runObjective,
} from '../src/commander/commander';
import { Rosbridge } from '../src/ros/rosbridge';
import { FakeSocket } from './fakeSocket';

describe('the commander over rosbridge', () => {
  let ros: Rosbridge;
  let socket: FakeSocket;

  beforeEach(() => {
    FakeSocket.all = [];
    vi.stubGlobal('WebSocket', FakeSocket);
    ros = new Rosbridge('ws://rig:9090');
    socket = FakeSocket.last;
    socket.open();
  });
  afterEach(() => vi.unstubAllGlobals());

  const end = (status: number, treeStatus: number, message: string) => {
    const goal = socket.lastSent('send_action_goal');
    socket.receive({
      op: 'action_result', id: goal.id, status, result: true,
      values: { node_status: { status: treeStatus }, return_message: message },
    });
  };

  it('runs an objective, which succeeds when its tree does', async () => {
    const run = runObjective(ros, 'TakeShot');
    expect(socket.lastSent('send_action_goal')).toMatchObject({
      action: ACTION, action_type: EXECUTE_TREE, args: { target_tree: 'TakeShot', payload: '' },
    });
    end(4, 2, 'Tree finished with status: SUCCESS');
    await expect(run).resolves.toEqual({ ok: true, message: 'Tree finished with status: SUCCESS' });
  });

  it('fails an objective whose tree failed, or that another replaced', async () => {
    const failed = runObjective(ros, 'TakeShot');
    end(6, 3, 'Tree finished with status: FAILURE');
    await expect(failed).resolves.toEqual({ ok: false, message: 'Tree finished with status: FAILURE' });

    const replaced = runObjective(ros, 'TakeShot');
    end(6, 0, "Preempted by objective 'ActivateTeleop'");
    await expect(replaced).resolves.toMatchObject({ ok: false, message: "Preempted by objective 'ActivateTeleop'" });
  });

  it('stops every objective, whoever sent it', async () => {
    const stop = cancelAll(ros);
    expect(socket.lastSent('call_service')).toMatchObject({
      service: '/commander/execute_objective/_action/cancel_goal', type: 'action_msgs/srv/CancelGoal', args: {},
    });
    socket.respond({ return_code: 0, goals_canceling: [] });
    await expect(stop).resolves.toBeUndefined();
  });

  // A page opened on any device knows what runs, whoever sent it: the
  // commander's latched topic gives the current name at once.
  it('follows the name of the running objective, latched', () => {
    const objective = vi.fn();
    followObjective(ros, objective);
    expect(socket.lastSent('subscribe')).toMatchObject({
      topic: '/stepit_server/objective', type: 'std_msgs/msg/String', qos: { durability: 'transient_local', reliability: 'reliable' },
    });
    socket.receive({ op: 'publish', topic: '/stepit_server/objective', msg: { data: 'FocusStack' } });
    socket.receive({ op: 'publish', topic: '/stepit_server/objective', msg: { data: '' } });
    expect(objective.mock.calls).toEqual([['FocusStack'], ['']]);
  });
});

describe('whether an objective runs', () => {
  it('is not known until the commander said, which counts as running for Stop', () => {
    expect(objectiveRuns(undefined)).toEqual({ busy: false, known: false });
  });

  it('knows from the latched objective of the commander', () => {
    expect(objectiveRuns('')).toEqual({ busy: false, known: true });
    expect(objectiveRuns('FocusStack')).toEqual({ busy: true, known: true });
  });
});
