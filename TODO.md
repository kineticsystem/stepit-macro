# TODO

Open questions and concerns about the rig's behaviors, objectives and
controllers, written down as they came up. The code builds, the tests pass and
the objectives were run on the robot: these are decisions we deferred, and
measurements we made while deferring them. Item 10 is the one to read before
running the tests.

## Commanding motion

### 1. A speed instead of a duration

**Done:** the objectives that move the robot now use `TrapezoidalTrajectory`,
and take `max_velocity` and `max_acceleration` instead of a `duration`. What
follows is the analysis that led there, about `CubicTrajectory`.

`CubicTrajectory` builds a trajectory with a single waypoint: the target
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
  `max_velocity` 18.85 rad/s and `acceleration` 12.57 rad/s²
  (`stepit.ros2_control.xacro`), so `{offset: 31.4, duration: 0.5}` asks for
  ~63 rad/s: the controller commands a trajectory the hardware cannot follow and
  the joint simply lags behind it.

Possible answer: accept a `velocity` in the payload as an alternative to
`duration`, with `duration = |offset| / velocity` and exactly one of the two
allowed. It is a port on `CubicTrajectory` plus a few lines and tests, no
structural change. Optionally also refuse a command that exceeds `max_velocity`.

A trapezoid is available for the same duration, if the shape of the motion ever
matters. The controller never generates a velocity profile: it interpolates
between the waypoints it is given (positions only gives linear, positions and
velocities cubic, adding accelerations quintic). The single waypoint we send
therefore becomes a cubic, of peak velocity `1.5 d/T` and peak acceleration
`6 d/T²`. Three waypoints -- end of acceleration, start of deceleration, target
-- describe a trapezoid instead, of cruise velocity `d/(T - ta)` and
acceleration `d/(r (1 - r) T²)`, where `ta = r T` is the time spent
accelerating. Measured on the robot, 1 rad in 2 s with `r = 0.25`:

| | peak velocity | peak acceleration | time at cruise |
|---|---|---|---|
| one waypoint (cubic) | 0.770 rad/s | 1.50 rad/s² | 0.50 s |
| three waypoints (trapezoid) | 0.714 rad/s | 1.33 rad/s² | 0.83 s |

Same arrival time, 11% less peak velocity and acceleration, so a duration the
cubic cannot honour may still be feasible as a trapezoid. It also makes our
setpoints agree with the trapezoid the MCU runs internally, instead of the MCU
chasing a bell curve.

`TrapezoidalTrajectory` now builds such a trajectory, though not for a given
duration: it goes as fast as the limits allow. `MoveJointsTo`, `OffsetJointsBy`
and `SpinTest` all use it, so the objectives take limits instead of a duration.
`CubicTrajectory` remains for a move that must take a given time.

### 2. Why the trajectory controller and not the position controller

Question raised: is a subset of joints impossible with the position controller,
or is something misconfigured in StepIt?

It is a property of the controller type, and it is now verified on the robot:

- `position_controller` is a `position_controllers/JointGroupPositionController`,
  configured with `joint1 … joint5` (`robot_description/config/controllers.yaml`).
  Controllers of that family take a `std_msgs/Float64MultiArray` on `~/commands`
  holding one value per configured joint, in order. The message has no field
  naming joints, so there is no way to express "only joint1": every command
  writes all five.
- `joint_trajectory_controller` names the joints inside the message and is
  configured with `allow_partial_joints_goal: true`, which is exactly why a
  subset works.

Publishing all five values, with only `joint1` changed and the other four set to
the positions just read from `/joint_states`, moves that joint alone: the other
four stayed at 0.0 without a twitch. The moved joint peaked at 1.047 rad/s over
0.35 rad, against the `sqrt(a d) = 1.048 rad/s` of the MCU's own trapezoid, i.e.
the fastest the hardware will go over that distance. So the workaround works,
and it is also the way to reach a position as fast as the robot can.

What it costs:

- **No completion feedback at all.** There is no action and no goal, so an
  objective ends when the command is published, not when the robot arrives.
  Recovering it means a behavior that waits on `/joint_states` for the target,
  with a position tolerance, a settled velocity and a timeout, which is
  reimplementing what the trajectory controller's goal tolerance already does.
- **No synchronisation.** Each joint runs its own profile at full acceleration,
  so a large and a small motion commanded together arrive at different times.
- **Cancelling stops nothing.** The controller goes on holding the last setpoint,
  so halting the tree does not halt the robot. The waiting behavior would have to
  publish the current position to stop it.

**Decision: keep the trajectory controller for now.** It is the only option that
gives completion feedback, joints that arrive together, and a cancel that
actually decelerates the robot, and none of the alternatives improved on all
three. A maximum-speed move no longer needs the position controller:
`MoveJointsTo` builds one with `TrapezoidalTrajectory` and still sends it to the
trajectory controller.

The other option, never tried, is to load several `JointGroupPositionController`
instances, each with its own `joints` list (e.g. one per joint). They claim
different command interfaces, so several can be active at once, and
`EnsureControllers` can activate the one matching the subset.

### 3. A goal succeeds without checking where the robot is

**Done in StepIt:** its `controllers.yaml` now sets a `goal` tolerance of
0.01 rad per joint and a `goal_time` of 0.5 s, so a goal succeeds only once
every joint is within 0.01 rad of its target, and fails if one is not 0.5 s
after the end of the trajectory. That is the check that aborted `SpinTest` when
it ran at 100% of the motors' limits. What follows is the situation before.

`controllers.yaml` configured no `constraints`, and in the trajectory controller
a tolerance of 0.0 means *disabled*, not *exact*. The per-joint `goal` tolerance
is therefore never applied, and the only check left is
`stopped_velocity_tolerance` (0.01 rad/s by default), which the controller folds
into the goal tolerance as its velocity component. The success condition of a
goal is thus "the last point of the trajectory is in the past and the joint is
not moving", with the position never compared.

For the durations we send, this is right by accident: at time `T` the robot is
genuinely there and stopped. It breaks as soon as a trajectory ends before the
motion does. Commanding a single waypoint with `time_from_start = 0`, measured on
the robot:

| Goal tolerances | Result | When |
|---|---|---|
| none, i.e. today's configuration | SUCCESS | 0.051 s, at 0.198 of 0.0, accelerating |
| `goal_tolerance.position = 0.01` | SUCCESS | 0.501 s, the true arrival |
| and `goal_time_tolerance = 0.1` | ABORT, tolerance violated | 0.201 s, stranded at 0.172 |

`FollowJointTrajectory` carries `goal_tolerance` and `goal_time_tolerance` in the
goal itself, so this is a field on the message we already send, not a change to
StepIt's configuration. A `goal_time_tolerance` of zero means "wait
indefinitely", which is what the second row does; a non-zero one aborts and
leaves the joint wherever it stopped, as the third shows.

## Safety and control flow

### 4. Cancelling is a controlled stop, not an emergency stop

Cancelling the `ExecuteTree` goal propagates correctly: the tree halts, the
behavior cancels the `FollowJointTrajectory` goal, and the controller holds the
position captured at that moment. But the robot decelerates at the configured
acceleration, so the overshoot grows with speed: measured 0.04 rad when creeping,
~1.7 rad when cancelled mid-motion at ~2.3 rad/s. A true emergency stop would be
a different mechanism, e.g. deactivating the controller or halting the hardware.

### 5. Switching controllers while a trajectory is running

`EnsureControllers` deactivates whatever is driving the robot. Deactivating the
trajectory controller *during* a motion has never been tested: it may or may not
stop the motors cleanly. If it matters, the objective should cancel the motion
first.

### 6. The objectives switch controllers unconditionally

`OffsetJointsBy`, `MoveJointsTo` and `SpinTest` all call `EnsureControllers`, so
they stop a controller that was activated on purpose. That is the self-healing behavior we
chose, but the alternative is to *check* the active controller and fail instead
of switching. One line of XML either way.

### 7. No timeout around the trajectory

If the trajectory controller accepts a goal and never finishes, the objective
runs until the client cancels. Wrapping `FollowJointTrajectory` in the built-in
`<Timeout msec="...">` decorator would bound it. XML only, no C++.

## Payload conventions

### 8. Scalars and lists are not symmetric

`{controllers: velocity_controller}` and `{controllers: [velocity_controller]}`
are both accepted, because those ports go through `stepit_behaviors::getNames`,
and so are `{offset: -1.0}` and `{offset: [-1.0]}`, through its numeric twin
`getNumbers`, like `max_velocity` and `max_acceleration`. A single joint name
works too, `{joints: joint1, offset: -1.0}`, as tried on the robot:
BehaviorTree.CPP converts the string into a list of one. `positions` is the
exception: `MoveJointsTo` passes it to `TrapezoidalTrajectory`, whose port only
takes a list, while `MoveJointsDirectlyTo` reads it through `getNumbers`.
Making it symmetric means routing the `positions` ports of `CubicTrajectory` and
`TrapezoidalTrajectory` through `getNumbers`, and declaring them
`BT::AnyTypeAllowed`. Small, but it is new API surface, so it was left alone.

### 9. Radians are assumed everywhere

`offset` and `positions` are documented as radians, and the limits as rad/s
and rad/s². Nothing in the behaviors is
bound to rotary joints, but a prismatic joint would carry metres in the same
field, with no way to tell them apart. Only a documentation problem today.

## Tests

### 10. The tests reach the robot

The fake robot and the fake controller manager of the tests use the real names:
`/joint_states`, `/joint_trajectory_controller/follow_joint_trajectory` and
`/controller_manager/...`. The robot and the commander's container, where the
tests run, share the host network, so on the default ROS domain the tests read
the real joint states, switch the real controllers and send goals to the real
trajectory controller: a test run once moved motors 1 and 2 of the robot.

**Done in part:** both `test.sh` scripts set `ROS_DOMAIN_ID` to 77, or to
`STEPIT_TEST_DOMAIN_ID`, overriding whatever the shell has, which is the
robot's. A plain `colcon test`, or a test binary run by hand, still runs on the
shell's domain. Setting the domain on each test, through the `ENV` argument of
`ament_add_gtest` in the two tests' `CMakeLists.txt`, would close that too.

## Worth considering

### 11. Node patterns from Nav2

There is no library of ready-made behaviors for `ros2_control` robots, but
`nav2_behavior_tree` has domain-agnostic control and decorator nodes worth
copying (Apache-2.0): `recovery_node` (run an action, run a recovery, retry) and
`rate_controller` (tick a branch at N Hz) are the two that would earn their place
here. They are built on Nav2's own base classes, so they would have to be ported
to `BehaviorTree.ROS2`, not linked against.

## Feedback

### 12. How far along a running objective is

**Done in part:** step 1 below, its catch included, and `Steps`: the commander
reports the progress of a node implementing `stepit_server::ProgressReporter`,
and the StepIt Editor shows it on the running rows. `FollowJointTrajectory`,
`CommandJointPositions` and the objective's percentage (step 3) remain. What
follows is the analysis, as it was before.

Nothing reports progress. The commander's feedback, a single
`string message` of `ExecuteTree`, is the JSON of `ExecutionStatus`
(`modules/stepit-commander/src/stepit_server/include/stepit_server/execution_status.hpp`):
the tree, then the status of every node that changed, at most every 50 ms. A
client sees *which* node runs, never *how far* it is. None of our behaviors
read the feedback of their own action either.

Most behaviors can know their progress:

| Behavior | Progress | From |
|---|---|---|
| `FollowJointTrajectory`, so every `TrapezoidalTrajectory` and `CubicTrajectory` move | exact | the `time_from_start` of the last point, known before the goal is sent, against the time elapsed or the `desired.time_from_start` of the controller's feedback |
| `Steps` | exact | `index` over `count`, both already ports |
| `CommandJointPositions` | estimate | the share of the distance covered, on `/joint_states`, start and target being known; or a time from the trapezoid of the motor limits (2 turns/s²). The MCU plans the move, so there is no timeline to read |
| `SwitchController`, `GetJointPositions`, … | not worth it | instantaneous |
| a camera trigger, once there is one | estimate | the exposure |

The objective as a whole has no percentage in general: a `Fallback`, a
condition, a retry or a loop of unknown length has no defined total, and the
commander must stay generic. Ours are plain sequences with fixed counts, though:
`Stack` is `(row_index · 11 + shot) / 121`, refined by the move in flight.

Possible answer, in three steps:

1. **Commander, generic:** an interface such as
   `ProgressReporter { virtual std::optional<double> progress() const; }`.
   `ExecutionStatus` asks every RUNNING node that implements it and adds
   `"progress": {"12": 0.42}` to the JSON, by `_uid`. Existing clients ignore
   the new key, and the commander still knows nothing about the rig. A node
   that counts, like `Steps`, could send `{"done": 3, "total": 11}` instead:
   "row 4 of 11" says more than 27%.

   **Catch:** `ExecutionStatus::feedback` sends nothing while no status
   changes, and during a 10 s move only one node runs and nothing changes: the
   progress would jump from 0 to 100% when the move ends. A changed progress
   must count as a change, still at most one message per period (5 Hz is
   plenty for a progress bar).
2. **`stepit_behaviors`:** implement it in `Steps` and `FollowJointTrajectory`,
   exact, and in `CommandJointPositions`, by distance.
3. **The objective:** the progress of the outermost running `Steps`, refined by
   that of its running child, computed by the client (e.g. the StepIt Editor)
   from the per-node values: it has the executed tree, the server need not.

**In the StepIt Editor** (`modules/stepit-editor`), the execution view is
already built on feedback by `_uid`, so it is small:

- `parseFeedback` (`src/client/execution.ts`) reads `progress` next to
  `nodes`. Today it ignores unknown keys, so step 1 breaks nothing.
- `applyFeedback` (`src/client/store/execution.ts`) keeps
  `execution.progress` next to `statuses`; `endExecution` drops that of the
  nodes it marks HALTED.
- `StatusCell` (`src/client/components/ExecutionPanel.tsx`) shows a bar, or
  "row 4 of 11", on a RUNNING row that has one; `RunState`, in the header,
  shows the objective's percentage of step 3.
- Tests in `tests/execution.test.ts`.

**Why not a topic** on which each behavior publishes its `_uid` and progress,
which was considered:

- **No run.** A `_uid` is unique within one tree only. With preemption, the
  last message of a halted objective can arrive after the next one started,
  and land on a node of the new tree with the same `_uid`. Fixing it needs the
  goal id in every message, which behaviors do not know.
- **Two unordered channels.** The statuses come by the action, the progress by
  the topic, and nothing orders them: 42% after HALTED, or a progress before
  the first message, the one with the tree, when the client cannot yet tell
  which row a `_uid` is.
- **ROS where none is needed.** `Steps` is a plain BehaviorTree.CPP control
  node, with no ROS node: it would need one to publish, and every behavior a
  publisher and a rate limit of its own.
- **Its one advantage already exists.** Action feedback is an ordinary topic,
  `/commander/execute_objective/_action/feedback`: a dashboard or a gamepad
  light can subscribe without sending a goal, and every message carries its
  goal id.

The action feedback gives one channel, per goal, ordered with the statuses,
throttled in one place, and behaviors that only implement `progress()`. Its
cost is a change in the commander's repository, a generic one. A topic remains
right for a progress tied to no run, e.g. a camera's buffer filling up: that is
the status of a device, not of a node.

The alternative without touching the commander is a `Script` node writing
`{@progress}` from the `Steps` indices, but every objective then keeps a
formula of its own in step with its loops.

A time left instead of a percentage is the same problem: exact for a
trajectory, an estimate for a direct move, and for a whole `Stack` it also
needs the duration of a photo, which does not exist yet.

Start with step 1, its catch included, and `Steps` alone: that already gives
`Stack` a correct percentage.

## Connecting the controllers

### 13. Choosing the serial port of a controller from the UI

Each driver opens one serial port, fixed at startup: StepIt reads `usb_port`
from the URDF, `/dev/ttyACM0` by default, and Freezer from its node
parameters, `/dev/ttyUSB0` by default. Neither notices an unplugged board. The
serial library throws "device disconnected?" on the next read, but StepIt only
moves its hardware to `UNCONFIGURED`, where it stays, and Freezer logs a
warning and keeps the dead port open. While the dead port is open, the kernel
cannot give its name back: the board, plugged in again, comes back as
`ttyUSB1`, and a retry on the old name fails.

The two drivers have the same base: the `framed-serial` submodule, at the same
commit, over the `serial` library, and a `connect()` that sends `Info` up to 5
times and checks the controller's name and protocol version. Only the name,
the version and the format of the response differ.

| | StepIt | Freezer |
|---|---|---|
| Board | Teensy 4.1, native USB, `ttyACM*`, USB ID `16c0:0483` | Arduino Nano, FTDI FT232R, `ttyUSB*`, USB ID `0403:6001` |
| Resets when the port opens | no | yes, its bootloader runs for about 0.65 s |
| Controller name, protocol | `STEPIT`, 1 | `FREEZER`, 2 |
| Who calls the driver | a ros2_control hardware interface, from the real-time loop of the controller manager | the driver's own ROS2 node |

**Considered: detection and reconnection by the drivers.** With `usb_port:
auto`, a driver would open each serial port, run the handshake, and keep the
first that answers; a thread would search again every second after an unplug.
It was set aside for now:

- **A scan disturbs the other boards.** Opening a port resets an Arduino and
  sends it a frame, even while another driver uses it: Linux does not lock a
  serial port. It needs a filter by USB ID and a lock on each port to be safe.
- **StepIt cannot wait in its loop.** `read()` and `write()` run in the
  real-time loop of the controller manager, which a search of 100 ms would
  stall, and ros2_control does not configure a hardware again by itself.
- **StepIt loses its positions.** A Teensy powered by USB restarts when
  unplugged, and its step counts with it. A reconnection that nobody asked
  for would carry on from positions that are wrong until the axes are homed.

**Chosen: a configuration panel in the UI.** We pick the port of each
controller, probe it, and retry by hand when something misbehaves. Nothing
scans, so no other board is touched, and a reconnection happens only when a
person asks for it, who can home the axes first. A reconnection thread can
still be added later, on the same interfaces. What it needs:

1. **`framed-serial`: the ports and a distinct error.** A function over
   `serial::list_ports()` that lists each port with its description and its
   USB ID, which Linux reports as e.g.
   `USB VID:PID=0403:6001 SNR=A700fkwd`, so that both drivers offer the same
   list. A `DisconnectedException` for an unplugged board: today a timeout
   throws `framed_serial::SerialException("timeout")` and an unplug the
   library's own `serial::SerialException`, which the drivers cannot tell
   apart without guessing.
2. **Each driver: close the port on an unplug.** On a `DisconnectedException`,
   the driver closes the port and reports itself disconnected, so that the
   panel does not show a dead connection as alive and a retry finds the board
   under its old name.
3. **Each driver: commands and a status.** A service listing the ports, one
   connecting to a given port, one disconnecting, and a latched topic of the
   state: connected or not, the port, the firmware, the last error. The panel
   shows the topic; its Probe and Retry buttons call the connect service.
4. **The UI: stable names.** It offers the names of `/dev/serial/by-id/`, made
   from the board's USB serial number, which do not change from one plug or
   boot to the next, e.g.
   `/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A700fkwd-if00-port0` for the
   Freezer Nano. A saved choice then always points at the same board,
   whatever `ttyUSB` or `ttyACM` number it gets. This works today, with no
   code: set `usb_port` to that path.

**In Freezer** (`modules/freezer-driver`) it is small: the node offers the
services and the topic itself, and its `connect()`, called once from the
constructor, becomes the connect service.

**In StepIt** (`modules/stepit-driver`) it is open. The port is a hardware
parameter of the URDF, read once, and the hardware interface has no services
of its own. The controller manager can already restart it, through
`set_hardware_component_state`, and `on_configure()` already connects, but
nothing gives it a new port: the URDF is fixed once loaded. `on_configure()`
could read the port from a file the UI writes, or from a parameter of a
helper node; what ros2_control on Jazzy offers needs checking first. Its
positions after a reconnection, see above, need a decision too: home the
axes, or mark the positions unknown until they are.

Start with steps 1 and 2 in `framed-serial` and Freezer, and the
`/dev/serial/by-id/` names, which already help. Leave the port change of
StepIt at runtime until the panel is built.
