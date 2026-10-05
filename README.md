# StepIt Macro

[![CI](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci.yml/badge.svg)](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci.yml)
[![Format](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci-format.yml/badge.svg)](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci-format.yml)
[![Linters](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci-ros-lint.yml/badge.svg)](https://github.com/kineticsystem/stepit-macro/actions/workflows/ci-ros-lint.yml)

> [!WARNING]
> This project is a work in progress and not fully implemented yet. Today it
> only controls the camera: the rail, the rotary stage and the lights are
> still to come.

An automated macro photography system for 3D focus stacking.

A camera is mounted on a motorized linear rail, which sits on a rotary stage.
For each angle, the rail moves the camera through a series of focus distances
and captures an image at each step. The stage then rotates to the next angle
and repeats, producing a full set of focus stacks around the subject.

The system also switches LED lights on and off during the shoot, so lighting
is consistent and synchronized with each capture.

## Table of Contents <!-- omit in toc -->

- [Features](#features)
- [The Modules](#the-modules)
- [The Camera's Test Page](#the-cameras-test-page)
- [Prerequisites](#prerequisites)
- [Install StepIt Macro](#install-stepit-macro)
  - [Check out the Git Repository](#check-out-the-git-repository)
  - [Build the Project](#build-the-project)
- [Running the Application](#running-the-application)
- [The Behaviors and Objectives](#the-behaviors-and-objectives)
  - [Packages](#packages)
  - [Objectives](#objectives)
  - [Adding an Objective](#adding-an-objective)
  - [Tests](#tests)
- [The Rig's Own Programs](#the-rigs-own-programs)
  - [The Gamepad](#the-gamepad)
  - [The Two Workspaces](#the-two-workspaces)
- [Working on a Module](#working-on-a-module)
- [Updating the Modules](#updating-the-modules)
- [How It Works](#how-it-works)
- [Continuous Integration](#continuous-integration)

## Features

- Automated focus stacking along a linear rail
- Multi-angle capture via rotary stage
- Synchronized LED lighting control
- Output suitable for focus-stack merging and 3D reconstruction

## The Modules

StepIt Macro runs the whole rig with one command. It brings together six
projects, checked out as git submodules under [`modules`](modules), five of
which run in a container:

| Module | Container | What it does |
|---|---|---|
| [StepIt Driver](https://github.com/kineticsystem/stepit-driver) | `stepit-driver` | The robot: ROS2 control of the stepper motors, with fake motors by default, and RViz on demand. |
| [StepIt Commander](https://github.com/kineticsystem/stepit-commander) | `stepit-commander` | The action server that runs *objectives*, behavior trees, and rosbridge on port 9090. The rig's own objectives and behaviors are in [`src/plugins`](src/plugins), see [The Behaviors and Objectives](#the-behaviors-and-objectives). |
| [StepIt Editor](https://github.com/kineticsystem/stepit-editor) | `stepit-editor` | The web editor of the objectives, on <http://localhost:8080>, which runs them on the robot through the commander. |
| [StepIt Camera](https://github.com/kineticsystem/stepit-camera) | `stepit-camera` | The ROS2 driver of the camera, over USB: live view, settings, and the download of every picture. It serves its test page and the pictures on <http://localhost:8090>, the live view through web_video_server on port 8081, and its own rosbridge on port 9091. |
| [Freezer Driver](https://github.com/kineticsystem/freezer-driver) | `freezer-driver` | The ROS2 driver of the Freezer board, an Arduino Nano that fires the cameras and the flashes with a hardware timer, with a fake controller by default. It serves its board page on <http://localhost:8092> and its own rosbridge on port 9092. |
| [StepIt UI](https://github.com/kineticsystem/stepit-ui) | none | The application of the whole rig, not started yet. |

Each module keeps its own Docker container and scripts; StepIt Macro only
starts them together and wires them up: the commander runs the rig's own
objectives, from [`src/plugins`](src/plugins), and the editor opens them, so a tree saved in the
editor is the tree the commander runs, and
the camera's test page reaches the camera through the camera's own servers.

## The Camera's Test Page

StepIt Camera comes with a test page, to try the camera from a browser:

- it shows what the camera sees, live;
- it sets the ISO, the shutter speed, the aperture and the white balance;
- it takes a test shot, and shows it.

It is not the application of the rig: that will be
[StepIt UI](https://github.com/kineticsystem/stepit-ui), with the rail, the
rotary stage and the lights too.

The page talks to the camera straight from the browser, so it needs no ROS
itself:

- through the camera's web server, which serves the page and the pictures;
- through [rosbridge](https://github.com/RobotWebTools/rosbridge_suite), a
  WebSocket that speaks JSON, to call the services, set the parameters and
  hear of the pictures;
- through [web_video_server](https://github.com/RobotWebTools/web_video_server),
  which streams the live view as MJPEG into a plain `<img>`.

All three run next to the camera driver, whose launch file starts them. See
[The Test Page](https://github.com/kineticsystem/stepit-camera#the-test-page)
in the README of StepIt Camera for how to use it.

## Prerequisites

We need a computer with Ubuntu 24.04 and Docker. Please refer to the document
[Install Docker Engine on Ubuntu](https://docs.docker.com/engine/install/ubuntu/),
or run:

```
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
```

The modules are cloned over SSH, so a GitHub account with an
[SSH key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh)
is required.

For the camera, we need a camera supported by
[libgphoto2](http://www.gphoto.org/proj/libgphoto2/support.php), such as a
Canon EOS 5D Mark II, connected over USB, with its AC adapter. See
[Prepare the Camera](https://github.com/kineticsystem/stepit-camera#prepare-the-camera)
in the README of StepIt Camera, e.g. the mode dial on M. Without a camera, the
rest of the rig runs anyway, and the camera driver waits for one.

The Freezer board is not needed either: its driver runs with a fake controller
by default. For the board, an Arduino Nano flashed with the firmware of the same
commit of Freezer Driver, see
[Install Freezer Driver on the Microcontroller](https://github.com/kineticsystem/freezer-driver#install-freezer-driver-on-the-microcontroller)
in its README, and a user in the group `dialout`.

> [!IMPORTANT]
> **Set _Auto power off_ to _Off_ in the camera's menu.** Otherwise the camera
> goes to sleep, as soon as a minute after the last use, and the driver loses
> it until it wakes up: the live view stops, and no picture is downloaded.

## Install StepIt Macro

### Check out the Git Repository

Check out this git repository, including all the modules and their own
submodules.

```
git clone --recurse-submodules git@github.com:kineticsystem/stepit-macro.git
cd stepit-macro
```

If you missed the switch `--recurse-submodules`, download the modules with:

```
./docker/dock.sh download
```

### Build the Project

Everything is driven by the [`docker/dock.sh`](docker/dock.sh) script, which can
be called from anywhere. Run it without arguments to list its commands.

> [!IMPORTANT]
> Each container provides a default user `developer` with password `developer`.

Build the images, then install the dependencies and compile the code of every
module inside its container. The first build takes a while: the robot's image
is based on ROS2 Jazzy desktop, and the editor's compiles BehaviorTree.CPP.

```
./docker/dock.sh build
```

The packages it downloads, for the images and for the dependencies of each
module (rosdep), are kept on this machine by `stepit-apt-cache`, a local proxy
that `build` starts, so that later builds take them from the disk. They are in
the Docker volume `stepit-macro_apt-cache`, which `clean` keeps; remove it with
`docker volume rm stepit-macro_apt-cache` to free the space. The dependencies
are installed into each image while compiling, so the containers start without
installing them again. See [The Package Cache](docs/PackageCache.md), which also
describes the cache of CI.

This is also how you pick up a change to a `Dockerfile` or to the code: it
rebuilds only the image layers that changed. To build a single module, name its
container: `stepit-driver`, `stepit-commander`, `stepit-macro` (the rig's own
programs, see [The Rig's Own Programs](#the-rigs-own-programs)),
`stepit-editor`, `stepit-camera`, or `freezer-driver`.

```
./docker/dock.sh build stepit-commander
```

## Running the Application

Ubuntu's desktop mounts a camera as soon as it is plugged in, and then holds
it, so that the camera driver cannot open it. Unmount it before starting, or
stop Ubuntu from doing it, as the
[troubleshooting](https://github.com/kineticsystem/stepit-camera#troubleshooting)
of StepIt Camera explains.

Start the whole rig in the background:

```
./docker/dock.sh start
```

The editor is on <http://localhost:8080>: open an
objective, e.g. `OffsetJointsBy`, and press **Run** to execute it on the robot.
The camera's test page is on <http://localhost:8090>, with the live view of the
camera. The pictures the camera takes are saved in
`modules/stepit-camera/pictures`. The Freezer's board page is on
<http://localhost:8092>, to fire a shot and see its timing.

The Freezer driver runs with a fake controller by default. To drive the board,
start it with `FREEZER_USE_FAKE=false`, and `FREEZER_USB_PORT` if the Nano is not
on `/dev/ttyUSB0` (restart it first if it is running):

```
FREEZER_USE_FAKE=false ./docker/dock.sh start freezer-driver
```

RViz does not open by default. To see the robot in RViz, start the driver with
`LAUNCH_RVIZ=true`, e.g. `LAUNCH_RVIZ=true ./docker/dock.sh start stepit-driver`
(restart it first if it is running).

Follow the output of every service, or of one of them. Stop following with
`Ctrl+C`: the services keep running.

```
./docker/dock.sh logs
./docker/dock.sh logs stepit-camera
```

Show which containers are running:

```
./docker/dock.sh status
```

Open a terminal into a container, e.g. to send an objective from the command
line:

```
./docker/dock.sh shell stepit-commander
```

```
source install/setup.bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: OffsetJointsBy,
    payload: '{joints: [joint1, joint3], offset: -6.28}'}"
```

Stop everything:

```
./docker/dock.sh stop
```

Finally, run this to remove the containers and their images:

```
./docker/dock.sh clean
```

To publish the editor on another port, set `EDITOR_PORT` when starting it,
e.g. `EDITOR_PORT=9000 ./docker/dock.sh start`.

## The Behaviors and Objectives

The objectives the rig runs, and the behaviors they are built from, are the
rig's own: they live here, in [`src/plugins`](src/plugins), not in StepIt
Commander, whose server runs any robot's. They are built as a ROS workspace of
their own, on top of the commander's, inside the commander's container, where
this repo is mounted at `~/rig`, see [The Two Workspaces](#the-two-workspaces). The server loads them from the folders listed in
[`commander.yaml`](src/plugins/stepit_objectives/config/commander.yaml), which the rig
passes to it as `params_file`.

### Packages

| Package | Role |
|---|---|
| `stepit_objectives` | The objectives and the subtrees they are built from: BehaviorTree XML files, no code, and the parameters of the server. `objectives/stepit_behaviors.xml` describes the behaviors for editors such as the StepIt Editor. |
| `stepit_behaviors` | The behaviors the objectives are built from. The only place that knows the topics, actions and services of the robot. |
| `stepit_tests` | Tests: the logic of the behaviors, and the objectives run end to end against a fake robot. |

### Objectives

Each objective is documented in its own file under [`docs`](docs), named after
the objective, i.e. after the `target_tree` of the command:

| Objective | What it does |
|---|---|
| [`OffsetJointsBy`](docs/OffsetJointsBy.md) | Moves joints **by** a signed offset, relative to where they are. |
| [`MoveJointsTo`](docs/MoveJointsTo.md) | Moves joints **to** absolute positions. |
| [`OffsetJointsDirectlyBy`](docs/OffsetJointsDirectlyBy.md) | Like `OffsetJointsBy`, through the position controller: each joint on the microcontroller's own profile, fastest, but not synchronised. |
| [`MoveJointsDirectlyTo`](docs/MoveJointsDirectlyTo.md) | Like `MoveJointsTo`, through the position controller: each joint on the microcontroller's own profile, fastest, but not synchronised. |
| [`ActivateController`](docs/ActivateController.md) | Stops the controller driving the robot and activates another one. |
| [`ActivateTeleop`](docs/ActivateTeleop.md) | Hands the robot to the gamepad: stops the controllers driving it and activates the velocity controller. The gamepad's stop button runs it. |
| [`SpinTest`](docs/SpinTest.md) | Hardware test: joint *k* turns *k* times clockwise at 90% of the motors' limits, then all return home. |
| [`Stack`](docs/Stack.md) | Steps joint1 and joint2 through a grid of 11 × 11 positions, 5 turns in 10 steps each, then returns every joint home; joints 3, 4 and 5 stay in place. |

Run one from a terminal in the commander's container, opened with
`./docker/dock.sh shell stepit-commander`, e.g. to turn `joint1` and `joint3` by
one turn clockwise:

```bash
source ~/rig/install/plugins/setup.bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: OffsetJointsBy,
    payload: '{joints: [joint1, joint3], offset: -6.28}'}"
```

The behaviors show every shape a behavior can take: a ROS action client
(`FollowJointTrajectory`), service clients (`GetActiveControllers`,
`SwitchController`), a subscriber (`GetJointPositions`), a publisher that waits
on a subscription (`CommandJointPositions`) and pure logic (`OffsetVector`,
`TrapezoidalTrajectory`). `Steps` is a decorator that loops over values.

Building a trajectory and following it are separate behaviors, and
`FollowJointTrajectory` sends whatever trajectory it is given to the controller.
Two nodes build one:

- `CubicTrajectory`: a single waypoint, reached at rest after a given duration.
  The controller joins it with a cubic, which reaches its peak acceleration only
  at the start and the end, and its top speed only halfway. No objective uses
  it; it is there for a move that must take a given time.
- `TrapezoidalTrajectory`: as fast as the limits allow, 2.7 turns/s and 1.8
  turns/s² by default, 90% of those of the StepIt motors: at 100% the motors
  trail their commands and arrive late. Each joint accelerates at the limit,
  cruises at top speed and brakes at the limit; all joints start and stop
  together. It needs the positions the joints start from, e.g. from
  `GetJointPositions`. Every objective that moves the robot through the
  trajectory controller uses it.

[`TODO.md`](TODO.md) records the decisions deferred about them, with the
measurements behind them.

### Adding an Objective

1. Write the XML in `src/plugins/stepit_objectives/objectives`. Nothing else to do: the
   folder is already loaded by the server, which reads the files again before
   each goal whenever one was added, changed or removed, so the next goal runs
   it, with no build and no restart. A step that more than one objective needs
   goes in a tree of its own, in the same folder, called with `<SubTree
   ID="..."/>`. Such a subtree takes its parameters from ports
   (`{controllers}`), so each caller can pass its own; only the objective a
   client asks for reads the payload (`{@controllers}`). See how
   `ActivateController` forwards its payload to `EnsureControllers`.

   An objective is the main tree of its file: its `<root>` names it in
   `main_tree_to_execute`, as the editor does when it creates one. A tree the
   file does not name is a subtree: it runs only inside another tree, and the
   commander refuses to run it on its own. The editor switches a tree between
   the two with **Kind**, in place.

   Declare the payload of the objective in a `<TreeNodesModel>` of its file, as
   the ports of a `<SubTree>` with the objective's ID: one `input_port` per
   entry, named after it, whose description ends with an example of its value,
   as YAML, after `e.g.`. Editors such as the StepIt Editor show it in the Run
   dialog; the server ignores it. A subtree declares its own ports the same way,
   as `ensure_controllers.xml` does:

   ```xml
   <TreeNodesModel>
     <SubTree ID="OffsetJointsBy">
       <input_port name="joints">the joints to move, e.g. joint1 or [joint1, joint2]</input_port>
     </SubTree>
   </TreeNodesModel>
   ```
2. If it needs a new behavior, add it to `src/plugins/stepit_behaviors` and register it
   in `stepit_behaviors::registerNodes`. It is picked up automatically, because
   the whole package is loaded as one plugin. Then rebuild the rig and
   regenerate the node models that editors read (`test_nodes_model` fails until
   you do):

   ```bash
   ~/rig/bin/plugins/build.sh
   source ~/rig/install/plugins/setup.bash
   ros2 run stepit_behaviors write_nodes_model ~/rig/src/plugins/stepit_objectives/objectives/stepit_behaviors.xml
   ```

   The running server loaded the behaviors when it started: restart it with
   `./docker/dock.sh stop stepit-commander` and `./docker/dock.sh start
   stepit-commander`.
3. Add a test to `src/plugins/stepit_tests`.
4. Document its parameters in `docs/<ObjectiveName>.md` and add it to the
   [Objectives](#objectives) table.

### Tests

The objective tests, e.g. `test_offset_joints_by_objective`, run the real
objective XML and the real behaviors against a fake robot that publishes
`/joint_states`, serves `FollowJointTrajectory` and follows the commands of the
position controller, so no hardware and no controller are needed. Run them in
the commander's container:

```bash
~/rig/bin/plugins/test.sh
```

> [!WARNING]
> The fake robot uses the names of the real one. With the StepIt robot running
> on the same network and ROS domain, the tests read its joint states, switch
> its controllers and **move it**. The test scripts therefore always run them
> on ROS domain 77, or on `STEPIT_TEST_DOMAIN_ID` if set, whatever
> `ROS_DOMAIN_ID` the shell has. Run them through the scripts, never with a
> plain `colcon test`, which runs them on the robot's domain.

## The Rig's Own Programs

The behaviors and objectives run inside the commander, as a plugin. The rig's
programs that run on their own, as ROS nodes next to the modules, run in the
`stepit-macro` container instead, whose image is defined here, in
[`docker/Dockerfile`](docker/Dockerfile).

### The Gamepad

A Logitech Dual Action gamepad drives the robot by hand: its sticks set the velocity of the joints, through the robot's velocity controller, and its button 1 stops whatever moves the robot and hands it to the gamepad. It runs in the `stepit-macro` container, from the package [`stepit_teleop`](src/stepit-macro/stepit_teleop).

It belongs to StepIt Macro, not to StepIt Driver, because it needs the commander: the stop button runs the objective [`ActivateTeleop`](docs/ActivateTeleop.md), which the commander runs in place of the running objective, and which switches the controllers. See [Driving the Robot with a Gamepad](docs/Gamepad.md), which also tells how to test the gamepad with `jstest-gtk`.

### The Two Workspaces

The rig's packages are two ROS workspaces, one per container, side by side in
[`src`](src):

| Workspace | Built and run in | Scripts | Output |
|---|---|---|---|
| [`src/plugins`](src/plugins) | `stepit-commander`, which loads it as a plugin | [`bin/plugins`](bin/plugins) | `build/plugins`, `install/plugins` |
| [`src/stepit-macro`](src/stepit-macro) | `stepit-macro` | [`bin/stepit-macro`](bin/stepit-macro) | `build/stepit-macro`, `install/stepit-macro` |

They cannot be one workspace: the plugin must be built against the commander's
own workspace, which only its container has, and two containers building into
the same `build` and `install` would undo each other's work.

Inside `stepit-macro` this repo is mounted at `~/ws`, and the scripts of
`bin/stepit-macro` are on the `PATH` and aliased as `update`, `build` and
`test`. Its packages:

| Package | Role |
|---|---|
| `stepit_teleop` | The gamepad: `gamepad_teleop` turns the sticks into velocities and the stop button into `ActivateTeleop`. |
| `stepit_macro_tests` | Tests of the packages above, e.g. the gamepad against a fake commander. |

This workspace also builds `btcpp_ros2_interfaces`, the type of the commander's
action, from the commander's own copy in `modules/stepit-commander`, so that
it always matches the commander's. Build it, and run its tests, which
`test.sh` runs on a ROS domain of their own, as for the plugin:

```
./docker/dock.sh build stepit-macro
./docker/dock.sh shell stepit-macro
test
```

A new program goes into `src/stepit-macro` as a package, and into the command of
`stepit-macro` in [`docker/docker-compose.yml`](docker/docker-compose.yml).

## Working on a Module

Each module is a complete git repository of its own under [`modules`](modules):
edit, commit and push it as usual. Its README tells how to build and test it.

Inside a container, opened with `./docker/dock.sh shell <service>`, the module is
mounted at `~/ws`, and its scripts are on the `PATH` and aliased as in the
module's own container: `update`, `build`, `test`, for the editor `serve`,
`dev` and `validate`, and for the camera `dev`, for its test page. The Freezer's
board page has no alias: `cd ~/ws/web && pnpm dev`.

After changing the code of a module, or of the rig's behaviors in [`src/plugins`](src/plugins),
which `./docker/dock.sh build stepit-commander` builds too, compile it and
restart its service:

```
./docker/dock.sh build stepit-driver
./docker/dock.sh stop stepit-driver
./docker/dock.sh start stepit-driver
```

The objectives need no build: the commander reads them again before each goal.

If a service stops right after starting, look at its output with
`./docker/dock.sh logs <service>`. `./docker/dock.sh shell <service>` still
opens a terminal into a stopped service, in a new container of the same image.

## Updating the Modules

StepIt Macro records the commit of each module it was tested with. To move
every module to the latest commit of its `main` branch:

```
git submodule update --remote --recursive
./docker/dock.sh build
```

Then commit the new commits of the modules in StepIt Macro:

```
git add modules
git commit -m "Update the modules"
```

## How It Works

[`docker/docker-compose.yml`](docker/docker-compose.yml) defines one service per
module, each extending the `dev` service of the module's own
`docker/docker-compose.yml`. The Dockerfiles, the mounts and the network
settings therefore stay defined in one place, the modules, and StepIt Macro
only overrides:

- the command, which runs the application instead of keeping an idle
  container: `ros2 launch robot_bringup launch.py`,
  `ros2 launch stepit_server commander.launch.py` with the rig's
  `params_file`, `ros2 launch stepit_camera camera.launch.py`,
  `ros2 launch freezer_node freezer.launch.py`, and `serve.sh` for the editor;
- the commander's mounts: this repo, at `~/rig`, whose
  [`src/plugins`](src/plugins) holds the rig's behaviors and objectives, built
  on top of the commander's workspace;
- the folder the editor opens: the rig's `src/plugins/stepit_objectives/objectives`,
  instead of the editor's examples, through `BEHAVIORS_DIR`, with this repo
  mounted at `~/rig`;
- a service of the rig's own, `stepit-macro`, on an image defined in
  [`docker/Dockerfile`](docker/Dockerfile): it mounts this repo at `~/ws`, and
  `/dev/input` from the host for the gamepad, and runs
  `ros2 launch stepit_teleop teleop.launch.py`.

The containers and images have the names each module's own `dock.sh` gives
them by default: `stepit-driver`, `stepit-commander`, `stepit-editor`,
`stepit-camera` and `freezer-driver`; the rig's own container and image are `stepit-macro`. Run
one system or the other, not both: remove
the containers made by a module's `dock.sh` before starting StepIt Macro, e.g.
with `./modules/stepit-driver/docker/dock.sh stepit-driver clean`, and the
other way round.

The robot, the commander, the rig's own programs, the camera and the Freezer use the host network, so they discover
each other over DDS. The web pages reach them from the browser: the editor
through the commander's rosbridge on `ws://localhost:9090`, the camera's test
page through the camera's web server on `http://localhost:8090`, its rosbridge
on `ws://localhost:9091` and its web_video_server on `http://localhost:8081`, the Freezer's board page
through its web server on `http://localhost:8092` and its rosbridge on `ws://localhost:9092`. Each module serves its own
rosbridge, because a rosbridge only knows the messages installed next to it.

## Continuous Integration

Three GitHub Actions workflows run on every push and pull request, as in the
modules:

| Workflow | What it checks |
|---|---|
| [`ci.yml`](.github/workflows/ci.yml) | Builds and tests both workspaces, each in a job of its own, with the scripts of [`bin`](bin), in a `ros:jazzy-ros-base` container. The job of `src/plugins` first builds the commander's workspace from the `stepit-commander` submodule. A third job, `objectives`, validates the objectives with the StepIt Editor's validator, see below. |
| [`ci-format.yml`](.github/workflows/ci-format.yml) | The pre-commit hooks that need no ROS: clang-format, black, codespell, and the checks of whitespace and files. |
| [`ci-ros-lint.yml`](.github/workflows/ci-ros-lint.yml) | The ament linters of every package: copyright, lint_cmake and cpplint. |

The build cannot use `industrial_ci`, as the modules do: it builds one
workspace from the sources of the repo, and `src/plugins` is built on top of
the commander's. CI checks out only the `stepit-commander` and `stepit-editor` submodules, over
HTTPS; the other modules have CI of their own.

The `objectives` job runs the editor's validator, `validate`, on
[`src/plugins/stepit_objectives/objectives`](src/plugins/stepit_objectives/objectives):
the editor's own checks, then BehaviorTree.CPP loading every file with the node
models of the rig's behaviors, built from the ROS package, the library and the
version the commander loads them with. A broken objective, e.g. an unknown node
or port, or a subtree that does not exist, fails the pull request instead of
being refused by the commander on the robot. Run the same check by hand in the
editor's container, where it validates that folder by default:

```
./docker/dock.sh shell stepit-editor
validate.sh
```

The workflows run locally with [Nektos `act`](https://github.com/nektos/act),
e.g. `act -W .github/workflows/ci.yml`.
