# TODO

Open questions and concerns, written down as they came up. Nothing here is a
known defect: the code builds, the tests pass and the objectives were run on the
robot. These are decisions we deferred.

## Commanding motion

### 1. A speed instead of a duration

`FollowJointTrajectory` builds a trajectory with a single waypoint: the target
positions, zero velocity on arrival, and `time_from_start = duration` (5 s by
default). We say *where* to end up and *when* to be there, never how fast, so
the speed falls out of the arithmetic:

| Command | Average speed |
|---|---|
| `{offset: 6.28, duration: 4.0}` | ~1.6 rad/s |
| `{offset: 0.10, duration: 4.0}` | ~0.025 rad/s |

The speed is not even constant: the controller eases in and out to land at zero
velocity, so the mid-motion peak is about twice the average (measured on the
robot: ~2.3 rad/s during a move whose average was 1.05 rad/s).

Two consequences:

- To move at a known speed, the client has to compute
  `duration = |offset| / speed` for every command.
- Nothing rejects the impossible. The motors are configured with
  `max_velocity` 31.4159 rad/s and `acceleration` 3.14159 rad/s²
  (`stepit.ros2_control.xacro`), so `{offset: 31.4, duration: 0.5}` asks for
  ~63 rad/s: the controller commands a trajectory the hardware cannot follow and
  the joint simply lags behind it.

Possible answer: accept a `velocity` in the payload as an alternative to
`duration`, with `duration = |offset| / velocity` and exactly one of the two
allowed. It is a port on `FollowJointTrajectory` plus a few lines and tests, no
structural change. Optionally also refuse a command that exceeds `max_velocity`.

### 2. Why the trajectory controller and not the position controller

Question raised: is a subset of joints impossible with the position controller,
or is something misconfigured in StepIt?

Reasoned from the configuration, **not yet verified on the robot**:

- `position_controller` is a `position_controllers/JointGroupPositionController`,
  configured with `joint1 … joint5` (`robot_description/config/controllers.yaml`).
  Controllers of that family take a `std_msgs/Float64MultiArray` on `~/commands`
  holding one value per configured joint, in order. The message has no field
  naming joints, so there is no way to express "only joint1": every command
  writes all five.
- `joint_trajectory_controller` names the joints inside the message and is
  configured with `allow_partial_joints_goal: true`, which is exactly why a
  subset works.

So this looks like a property of the controller type rather than a
misconfiguration. Options, if position control on a subset is ever wanted:

- Read the current positions and resend them for the joints that must not move.
  That is what our tree already does through `GetJointPositions`.
- Load several `JointGroupPositionController` instances, each with its own
  `joints` list (e.g. one per joint). They claim different command interfaces, so
  several can be active at once, and `EnsureControllers` can activate the one
  matching the subset.
- Keep using the trajectory controller, which is the current answer.

To settle it: activate `position_controller` and publish to
`/position_controller/commands` with five values, then with fewer, and see what
the controller manager says.

## Safety and control flow

### 3. Cancelling is a controlled stop, not an emergency stop

Cancelling the `ExecuteTree` goal propagates correctly: the tree halts, the
behavior cancels the `FollowJointTrajectory` goal, and the controller holds the
position captured at that moment. But the robot decelerates at the configured
acceleration, so the overshoot grows with speed: measured 0.04 rad when creeping,
~1.7 rad when cancelled mid-motion at ~2.3 rad/s. A true emergency stop would be
a different mechanism, e.g. deactivating the controller or halting the hardware.

### 4. Switching controllers while a trajectory is running

`EnsureControllers` deactivates whatever is driving the robot. Deactivating the
trajectory controller *during* a motion has never been tested: it may or may not
stop the motors cleanly. If it matters, the objective should cancel the motion
first.

### 5. The objectives switch controllers unconditionally

`OffsetJointsBy` and `MoveJointsTo` both call `EnsureControllers`, so they stop a
controller that was activated on purpose. That is the self-healing behavior we
chose, but the alternative is to *check* the active controller and fail instead
of switching. One line of XML either way.

### 6. No timeout around the trajectory

If the trajectory controller accepts a goal and never finishes, the objective
runs until the client cancels. Wrapping `FollowJointTrajectory` in the built-in
`<Timeout msec="...">` decorator would bound it. XML only, no C++.

## Payload conventions

### 7. Scalars and lists are not symmetric

`{controllers: velocity_controller}` and `{controllers: [velocity_controller]}`
are both accepted, because those ports go through `commander_behaviors::getNames`.
`joints` and `positions` must always be lists: `{joints: joint1, offset: -1.0}`
fails. Making them symmetric means routing `FollowJointTrajectory`'s ports
through `getNames` and adding a numeric twin of it. Small, but it is new API
surface, so it was left alone.

### 8. Radians are assumed everywhere

`offset` and `positions` are documented as radians. Nothing in the behaviors is
bound to rotary joints, but a prismatic joint would carry metres in the same
field, with no way to tell them apart. Only a documentation problem today.

## Tooling and operations

### 9. The hooks need the container

The three ament linters come from the ROS workspace, so `git commit` on the host
fails unless they are skipped:

```bash
SKIP=ament_copyright,ament_lint_cmake,ament_cpplint git commit ...
```

Committing from inside the container is the intended path: it has `pre-commit`,
`clang-format` and ROS. `pre-commit install` has not been run in either place.

### 10. No CI

StepIt has three GitHub Actions workflows (industrial_ci, format, ros-lint).
This repository has none, so nothing checks a pull request. The hooks and
`./bin/test.sh` already define what CI would have to run.

### 11. Two servers collide on the Groot2 port

Running a second `commander_server` makes every goal fail with
`Behavior Tree exception: Address already in use`, because both try to publish
Groot2 on port 1667. Worth knowing before debugging the tree itself.

### 12. The submodule tracks a branch

`modules/BehaviorTree.ROS2` is pinned to a commit, as git always does, but
`.gitmodules` names the branch `humble`, so `git submodule update --remote`
would move it. There is no released Debian package to depend on instead.

### 13. The Dockerfile carries the container passwords

`developer:developer` and `root:docker` are in `docker/Dockerfile`, inherited
from the StepIt template. Intentional for a development container, but visible
to anyone who can read the repository.

## Worth considering

### 14. Node patterns from Nav2

There is no library of ready-made behaviors for `ros2_control` robots, but
`nav2_behavior_tree` has domain-agnostic control and decorator nodes worth
copying (Apache-2.0): `recovery_node` (run an action, run a recovery, retry) and
`rate_controller` (tick a branch at N Hz) are the two that would earn their place
here. They are built on Nav2's own base classes, so they would have to be ported
to `BehaviorTree.ROS2`, not linked against.
