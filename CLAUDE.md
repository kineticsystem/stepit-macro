# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

StepIt Macro runs the whole focus stacking rig **in one container**, `stepit-macro`: it builds the
git submodules under `modules/` (motors, commander, editor, camera, Freezer) and StepIt UI, the
rig's own page in `ui/`, and starts them all with one launch file, `stepit_bringup/rig.launch.py`,
configured by one file, `src/stepit-macro/stepit_bringup/config/rig.yaml`. Each module keeps its own container, tests and
CI, to work on it alone. The repo also holds the rig's **own ROS packages** in `src/`, as two
workspaces: `src/plugins`, the behaviors and objectives the commander loads, and
`src/stepit-macro`, the rig's own programs, e.g. the gamepad, and the launch file of the rig.
StepIt Commander is a generic server and knows nothing about the rig: never move rig behaviors or
objectives back into it.

## Working environment

Everything is built, tested and run **inside the container**, never on the host.

```bash
./docker/dock.sh build    # the image, then update.sh + build.sh of the whole rig in the container
./docker/dock.sh start    # the rig, and the package cache
./docker/dock.sh shell    # a terminal in the container, every workspace sourced
./docker/dock.sh logs
./docker/dock.sh stop
```

`dock.sh build` starts `apt-cache` first (`docker/apt-cache`, an apt-cacher-ng proxy on port 3142
whose packages live in the volume `stepit-macro_apt-cache`): the image build and every rosdep install
download through it. It compiles the rig in a container that it then commits as the image, so the
image holds the rosdep packages and the `~/.dependencies` marker and the rig does not run
`update.sh` again on start.

The container is privileged and mounts `/dev`, but its user (the host's uid) opens a device only
if the host lets it: it is in `dialout` (20, the serial ports) in the image, and joins the
host's `input` (the gamepad) and `plugdev` (the cameras, given by
`docker/udev/60-stepit-camera.rules` on a host without a desktop, e.g. the Pi) through
`group_add`, with the numbers `dock.sh` reads from the host: they differ between systems.
Start the rig with `dock.sh`, never a plain `docker compose up`, which lacks them.

The repo is mounted at `~/ws`. Three workspaces, each with its scripts in `bin/<workspace>/`,
building into `build/<workspace>`, `install/<workspace>` and `log/<workspace>` and naming its
folders explicitly: colcon never crawls the repo, and `modules/COLCON_IGNORE` keeps it out of the
submodules anyway. `bin/update.sh`, `bin/build.sh` and `bin/test.sh` (on the `PATH`, aliased
`update`, `build`, `test`) run them in order:

| Workspace | Holds |
|---|---|
| `modules` | The modules' ROS packages, the editor's web page (pnpm) and its validator, into the rig's own build folders, never the modules' (their own containers mount them at `~/ws`: the CMake caches differ). Not tested here: the modules have their own CI. The driver and the Freezer both carry `serial` and `framed-serial`: the driver's are built, and `bin/modules/build.sh` fails if the two pin different commits. `bin/modules/build.sh --packages-up-to <pkg>` builds part of it. |
| `plugins` | `src/plugins`, **on top of `modules`**, which holds the commander: the commander loads the plugin into its process, so it must be built against the commander's BehaviorTree libraries. `UNDERLAY` (an install folder) overrides the underlay: CI sets it to the commander's own workspace. |
| `stepit-macro` | `src/stepit-macro`, on ROS alone, so that CI builds it without the modules. |

`ui/` is not a workspace but a pnpm project, StepIt UI, with `bin/ui/` scripts of its own that the
three top scripts run last; it builds into `ui/dist`, which the rig serves.

```bash
~/ws/bin/plugins/test.sh    # on ROS domain 77 (or STEPIT_TEST_DOMAIN_ID): never a plain colcon test
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: OffsetJointsBy, payload: '{joints: [joint1], offset: -6.28}'}"
```

**Configuration.** `rig.yaml` has a section `launch`, the launch arguments of each module's launch
file (`robot`, `commander`, `camera`, `freezer`, `teleop`, and `editor`, which is not a launch file),
and node sections, an ordinary ROS2 parameter file that `rig.launch.py` writes to a temporary file
and passes as `params_file` to the commander, the camera and the Freezer, after their own. The
serial ports, fake or real hardware and the ports of the pages are set there; never edit a
module to configure the rig. An argument a module's launch file does not declare is an error, so
a typo cannot be ignored silently. Each include is a `GroupAction(scoped=True, forwarding=False)`:
arguments with the same name, e.g. `usb_port` or `web_port`, must never leak from one module to
the next. A change to `rig.yaml` needs a restart, no build.

**StepIt UI** (`ui/`, port 8070) is the application of the rig, part of this repo, not a module:
it works only against the rig. Its architecture, and a review of it, is in
`ui/docs/ARCHITECTURE.md`. It is for a tablet or a desktop; the test pages of the camera and of the Freezer are turned off in `rig.yaml`. Only the
robot's **tasks** go through the commander, as objectives (`TakeShot`, `ActivateTeleop`, moves):
**configuration** (the camera's settings, the live view, the lights) goes straight to the drivers,
so that it never preempts a running objective. A driver function becomes a behavior when an
objective needs it, not because the UI uses it. The sliders never publish velocities: the page
sends `sensor_msgs/Joy` on `/ui/joy` at 20 Hz while one is held, and `ui_teleop`, a second
`gamepad_teleop` (section `ui_teleop` of `rig.yaml`), turns it into velocities and stops the joints
when it stops coming for 0.5 s; never bypass that watchdog. Every page uses the commander's
rosbridge (9090): it knows every message of the rig. The image is `ros:jazzy-ros-base` (amd64 and
arm64, for the Raspberry Pi 5), not a desktop image: rosdep installs the rest, RViz included.

The commander loads the folders listed in the section `stepit_server` of `rig.yaml`,
`stepit_behaviors/bt_plugins` and `stepit_objectives/objectives`. The editor opens
`src/plugins/stepit_objectives/objectives` directly (`BEHAVIORS_DIR`). An edited or new XML runs on
the next goal with no build; a new or changed behavior needs `~/ws/bin/plugins/build.sh` and a
restart of the rig (`./docker/dock.sh stop && ./docker/dock.sh start`).

## Architecture

| Package | Rule |
|---|---|
| `stepit_objectives` | XML only, no code: objectives, the subtrees they reuse, the generated node models, all in `objectives/`. |
| `stepit_behaviors` | The **only** place the objectives name robot topics, actions and services. |
| `stepit_tests` | All tests of the behaviors and objectives; the other packages carry none. |

**`src/stepit-macro`** holds the rig's own programs, ROS nodes that run on their own rather than
inside the commander, and the launch file of the rig. It also builds `btcpp_ros2_interfaces` from
`modules/stepit-commander`, the type of the commander's action, which has no Debian package. Its
tests go in `stepit_macro_tests`. A new program needs a launch file, an entry in `MODULES` of
`rig.launch.py` and its section in `rig.yaml`.

| Package | Rule |
|---|---|
| `stepit_bringup` | `rig.launch.py` and `config/rig.yaml`: the only place the rig starts and configures the modules. Installed as links (`--symlink-install`): the launch file finds the repo from its source. |
| `stepit_teleop` | The gamepad (`gamepad_teleop`): sticks to `/velocity_controller/commands`; stop button to the objective named by its `objective` parameter, `ToggleTeleop`, which the commander runs in place of the running one: `ActivateTeleop`, or the trajectory controller back when the gamepad already drives the robot. The switching logic lives in that objective, not in the node. See `docs/Gamepad.md`. |
| `stepit_power` | `power_off`: the service `~/power_off` behind StepIt UI's power button. It switches the computer off with `busctl` (systemd-logind, over the host's `/run/dbus`, mounted by `docker-compose.yml`), refused while an objective of `refuse_during` runs (`FocusStack`, `Stack`). logind allows it only with `docker/polkit/50-stepit-power-off.rules` installed on the rig's computer: never on a development PC, and never let a test run the real command (tests pass a command of their own). |
| `stepit_state` | `stack_state`: the state of the rig, its parameters `state.*`. The only writer of the state file. |
| `stepit_macro_tests` | All tests of `src/stepit-macro`, `rig.yaml` included. |

**Nothing is wired up by hand.** An objective is an XML file dropped into
`stepit_objectives/objectives`, whose `<root>` names it in `main_tree_to_execute`; a tree it
does not name is a subtree (e.g. `EnsureControllers`), which the commander refuses to run on its
own. A new behavior needs a line in `stepit_behaviors::registerNodes`
(`src/register_nodes.cpp`) and a regenerated node model; `plugin.cpp` exports the whole package
as one `BT_PLUGIN_EXPORT` plugin, installed into `share/stepit_behaviors/bt_plugins`.

**Action behaviors** derive from `stepit_behaviors::RosActionNode` (`ros_action_node.hpp`), not
from `BT::RosActionNode` directly: halting a node whose goal has just ended makes BehaviorTree.ROS2
throw `UnknownGoalHandleError`, which would end the commander's process. `Shoot` sends goals to
StepIt Freezer, and `ExpectPicture` reads StepIt Camera's `/camera/picture`, so the plugin needs
`freezer_msgs` and `stepit_camera_msgs`: CI builds them from `modules/stepit-freezer` and
`modules/stepit-camera` on top of the commander's workspace. An objective cannot have the name of a
behavior (BehaviorTree.CPP refuses it): the objective of `Shoot` is `TakeShot`.

**Parameters of the behaviors** (the overshoot of each motor against backlash, `overshoot.<joint>`,
the millimetres a linear axis travels per motor turn, `mm_per_turn.<joint>`, the degrees a rotary
axis turns per motor turn, `deg_per_turn.<joint>`, and `pictures_folder`, the
camera's `download_directory` through a YAML anchor, for `StackDone` and `AllStacksDone`) go in the section
`stepit_server` of `rig.yaml`. The plugin reads them from the commander's parameter overrides
(`parameters.cpp`) and **never declares or sets a parameter on the commander's node**:
BehaviorTree.ROS2 registers every plugin and tree again before the next goal after any change of
that node's parameters, a declaration included, which drops the objectives added since the last
build. Configuration measured by hand goes there; what the objectives learn while the rig runs,
e.g. the marks of a focus stack, is the state of the rig (below), never in `rig.yaml`.

**Every page shows the same.** StepIt UI keeps nothing of the rig in the browser: the state of a
stack lives on the rig, in the node `stack_state` (`stepit_state`), as its parameters `state.*`,
which `SaveValues`, `LoadValues` and the pages read and set through its parameter services: the
values of `saved` (the counts) in its state file, `~/ws/state/stack.yaml` in the git-ignored
folder `state`, and those of `forgotten` (the marks, counts of motor steps) in memory only, empty
at every start; the
commander publishes the running objective on `/stepit_server/objective`, and the whole run, every node
with its status, on `/stepit_server/execution`, which the editor's Execution tab follows, `FocusStack` its progress
on `/focus_stack/progress` (`ReportProgress`), both latched; each angle of a stack whose pictures are
all saved on `/focus_stack/stack_done` (`StackDone`), latched, with `stack.json` written into its
folder; each stack whose angles are all done on `/focus_stack/all_stacks_done` (`AllStacksDone`),
latched, with `all_stacks.json` written into its folder; and every page shows every picture on
`/camera/picture`. A new piece of shared state goes the same way, never into `localStorage`, and
never onto the commander's node: a new value is a name in `saved` or `forgotten` of the section
`stack_state` of `rig.yaml`.

**Node models for editors.** `stepit_objectives/objectives/stepit_behaviors.xml` is the
`<TreeNodesModel>` of every behavior, generated by `stepit_behaviors::nodesModel()` from the
real registration. Editors such as the StepIt Editor read it because they cannot load the
plugin, and it sits next to the objectives so that opening that one folder shows every node
type. The server does not read it: `stepit_objectives/CMakeLists.txt` excludes it from the
install, so it never reaches the installed folder listed in `behavior_trees`.
`test_nodes_model` fails when it is out of date; regenerate it with
`ros2 run stepit_behaviors write_nodes_model ~/ws/src/plugins/stepit_objectives/objectives/stepit_behaviors.xml`.

**Payload.** The commander writes the payload of a goal into the **global** blackboard, which is
why objectives read it with the `@` prefix (`{@joints}`), while values passed between nodes of
one tree have no prefix (`{current_positions}`). See the commander's README for the typing.

**Controller switching** lives in the `EnsureControllers` subtree, which every motion objective
calls first. It stops only controllers owning a command interface, so broadcasters keep
running, and it does not use `FORCE_AUTO` strictness: on StepIt each joint exports both a
position and a velocity interface, so the controller manager would leave both controllers
active. These are measured constraints, not preferences — see `docs/ActivateController.md`
before changing them.

**Motion** is built in two steps: a node writes a `trajectory_msgs/JointTrajectory` to the
blackboard, and `FollowJointTrajectory` sends it to the trajectory controller. Every objective
that moves uses `TrapezoidalTrajectory` (as fast as the limits allow, 90% of the motors' by
default); `CubicTrajectory` (one waypoint after a given duration) is kept for a timed move.
The direct objectives, `MoveJointsDirectlyTo` and `OffsetJointsDirectlyBy`, skip the trajectory:
`CommandJointPositions` sends every joint of `position_controller` its target (the others where
they are) and waits on `/joint_states` until the moved ones have arrived and stopped, so the
microcontroller plans each move alone. Faster, not synchronised. A halt deactivates
`position_controller` (asynchronously), and `StepitHardware` sends velocity 0 to the released
joints; never stop by sending the current positions, a moving joint brakes past them and comes back.
Velocity 0, from a release or from `/velocity_controller/commands`, is a smooth stop: the firmware
(AccelStepper `stop()`) and the fake motor decelerate at the motor's acceleration, 2 turns/s².

**The commander preempts** (its parameter `preempt`, on by default): a goal sent while an objective
runs replaces it, halted at its next tick. Stopping the robot is therefore asking for another
objective, as the gamepad's stop button does with `ActivateTeleop`; to stop without starting
anything, cancel every goal: `ros2 service call /commander/execute_objective/_action/cancel_goal
action_msgs/srv/CancelGoal "{}"`.

**Tests** run the real objective XML and the real behaviors against a fake robot
(`src/plugins/stepit_tests/tests/fake/fake_robot.hpp`, `fake_controller_manager.hpp`), so no hardware and
no controller manager are needed. New behaviors and objectives are expected to be covered the
same way. The fakes use the real topic, action and service names, so on the robot's ROS domain
they reach the real robot: both `test.sh` scripts therefore set `ROS_DOMAIN_ID` to 77, or to
`STEPIT_TEST_DOMAIN_ID`, overriding the shell's. Run the tests through them, never with a plain
`colcon test`.

## CI

`.github/workflows`: `ci.yml` builds and tests each workspace with the `bin/<workspace>` scripts in
`ros:jazzy-ros-base`, except `modules` (the modules have their own CI; the `plugins` job builds
`modules/stepit-commander` first, checked out over HTTPS: `.gitmodules` lists SSH URLs, and passes
it as `UNDERLAY`); `ci-format.yml` runs pre-commit without the ament hooks;
`ci-ros-lint.yml` runs those, per package. The `ui` job of `ci.yml` runs `bin/ui/` with Node.js, no
ROS. The `objectives` job of `ci.yml` builds the editor's native
validator (submodule `stepit-editor`) against the ROS package of BehaviorTree.CPP and runs
`validate` on `src/plugins/stepit_objectives/objectives`: a broken objective fails CI. A new package must be added to the package list
of `ci-ros-lint.yml` and to the paths of the ament hooks in `.pre-commit-config.yaml`.

## Conventions

- Every source file carries the MIT copyright header (`ament_copyright` enforces it).
- Packages compile with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`; C++17.
- `cpplint` runs with `--linelength=121`; `clang-format` uses the repo `.clang-format`.
- Each objective declares its payload in a `<TreeNodesModel>` of its file, as the ports of a
  `<SubTree>` with its ID: one `input_port` per `@key`, whose description ends with an example
  of its value, as YAML, after `e.g.`. Editors show it; the server ignores it. Keep it in step
  with the `{@key}` the objective reads.
- Each objective's parameters are documented in `docs/<ObjectiveName>.md` and listed in the
  README; update both when adding or changing an objective.
- Changing a module means committing in its own repository, then updating its pointer here
  (`git add modules/<module>`).
