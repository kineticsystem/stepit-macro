# StepIt Macro

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
- [The Control Panel](#the-control-panel)
- [Prerequisites](#prerequisites)
- [Install StepIt Macro](#install-stepit-macro)
  - [Check out the Git Repository](#check-out-the-git-repository)
  - [Build the Project](#build-the-project)
- [Running the Application](#running-the-application)
- [Working on a Module](#working-on-a-module)
- [Updating the Modules](#updating-the-modules)
- [How It Works](#how-it-works)

## Features

- Automated focus stacking along a linear rail
- Multi-angle capture via rotary stage
- Synchronized LED lighting control
- Output suitable for focus-stack merging and 3D reconstruction

## The Modules

StepIt Macro runs the whole rig with one command. It brings together five
projects, checked out as git submodules under [`modules`](modules):

| Module | Container | What it does |
|---|---|---|
| [StepIt](https://github.com/kineticsystem/stepit) | `stepit` | The robot: ROS2 control of the stepper motors, with fake motors by default, and RViz. |
| [StepIt Commander](https://github.com/kineticsystem/stepit-commander) | `stepit-commander` | The action server that runs *objectives*, behavior trees, on the robot, and rosbridge on port 9090. |
| [StepIt Editor](https://github.com/kineticsystem/stepit-editor) | `stepit-editor` | The web editor of the objectives, on <http://localhost:8080>, which runs them on the robot through the commander. |
| [StepIt Camera](https://github.com/kineticsystem/stepit-camera) | `stepit-camera` | The ROS2 driver of the camera, over USB: live view, settings, and the download of every picture. It serves web pages through web_video_server on port 8081 and its own rosbridge on port 9091. |
| [StepIt UI](https://github.com/kineticsystem/stepit-ui) | `stepit-ui` | The control panel of the rig, on <http://localhost:8090>. |

Each module keeps its own Docker container and scripts; StepIt Macro only
starts them together and wires them up: the editor opens the commander's
objectives, so a tree saved in the editor is the tree the commander runs, and
the control panel reaches the camera through the camera's own servers.

## The Control Panel

The control panel has one section per part of the rig. Today there is one, the
**Camera**:

- it shows what the camera sees, live;
- it sets the ISO, the shutter speed, the aperture and the white balance;
- it takes a test shot, and shows it.

The rail and the rotary stage, driven through StepIt Commander, and the lights
will come as other sections.

The control panel talks to the rig straight from the browser, so it needs no
ROS itself:

- through [rosbridge](https://github.com/RobotWebTools/rosbridge_suite), a
  WebSocket that speaks JSON, to call the services, set the parameters and
  receive the pictures;
- through [web_video_server](https://github.com/RobotWebTools/web_video_server),
  which streams the live view as MJPEG into a plain `<img>`.

Both run next to the camera driver, whose launch file starts them. See the
README of [StepIt UI](https://github.com/kineticsystem/stepit-ui) for how to
use it.

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
in the README of StepIt Camera: the mode dial on M, and _Auto power off_ set to
_Off_. Without a camera, the rest of the rig runs anyway, and the camera driver
waits for one.

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

This is also how you pick up a change to a `Dockerfile` or to the code: it
rebuilds only the image layers that changed. To build a single module, name its
container: `stepit`, `stepit-commander`, `stepit-editor`, `stepit-camera` or
`stepit-ui`.

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

RViz opens with the robot. The editor is on <http://localhost:8080>: open an
objective, e.g. `OffsetJointsBy`, and press **Run** to execute it on the robot.
The control panel is on <http://localhost:8090>, with the live view of the
camera. The pictures the camera takes are saved in
`modules/stepit-camera/pictures`.

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

To publish the editor or the control panel on another port, set `EDITOR_PORT`
or `UI_PORT` when starting them, e.g. `EDITOR_PORT=9000 ./docker/dock.sh start`.

## Working on a Module

Each module is a complete git repository of its own under [`modules`](modules):
edit, commit and push it as usual. Its README tells how to build and test it.

Inside a container, opened with `./docker/dock.sh shell <service>`, the module is
mounted at `~/ws`, and its scripts are on the `PATH` and aliased as in the
module's own container: `update`, `build`, `test`, for the editor `serve`,
`dev` and `validate`, and for the control panel `serve` and `dev`.

After changing the code of a module, compile it and restart its service:

```
./docker/dock.sh build stepit
./docker/dock.sh stop stepit
./docker/dock.sh start stepit
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
  `ros2 launch stepit_server commander.launch.py`,
  `ros2 launch stepit_camera camera.launch.py`, and `serve.sh` for the editor
  and the control panel;
- the folder the editor opens: the commander's
  `src/stepit_objectives/objectives`, instead of the editor's examples.

The containers and images have the names each module's own `dock.sh` gives
them by default: `stepit`, `stepit-commander`, `stepit-editor`,
`stepit-camera` and `stepit-ui`. Run one system or the other, not both: remove
the containers made by a module's `dock.sh` before starting StepIt Macro, e.g.
with `./modules/stepit/docker/dock.sh stepit clean`, and the other way round.

The robot, the commander and the camera use the host network, so they discover
each other over DDS. The web pages reach them from the browser: the editor
through the commander's rosbridge on `ws://localhost:9090`, the control panel
through the camera's rosbridge on `ws://localhost:9091` and its
web_video_server on `http://localhost:8081`. Each module serves its own
rosbridge, because a rosbridge only knows the messages installed next to it.
