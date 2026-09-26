# StepIt Workbench

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Install the StepIt Workbench](#install-the-stepit-workbench)
  - [Check out the Git Repository](#check-out-the-git-repository)
  - [Build the Project](#build-the-project)
- [Running the Application](#running-the-application)
- [Working on a Module](#working-on-a-module)
- [Updating the Modules](#updating-the-modules)
- [How It Works](#how-it-works)

## Introduction

The StepIt Workbench runs the whole StepIt system with one command. It brings
together three projects, checked out as git submodules under [`modules`](modules):

| Module | Container | What it does |
|---|---|---|
| [StepIt](https://github.com/kineticsystem/stepit) | `workbench-stepit` | The robot: ROS2 control of the stepper motors, with fake motors by default, and RViz. |
| [StepIt Commander](https://github.com/kineticsystem/stepit-commander) | `workbench-commander` | The action server that runs *objectives*, behavior trees, on the robot, and rosbridge on port 9090. |
| [StepIt Editor](https://github.com/kineticsystem/stepit-editor) | `workbench-editor` | The web editor of the objectives, on <http://localhost:8080>, which runs them on the robot through the commander. |

Each module keeps its own Docker container and scripts; the workbench only
starts the three together and wires them up: the editor opens the commander's
objectives, so a tree saved in the editor is the tree the commander runs.

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

## Install the StepIt Workbench

### Check out the Git Repository

Check out this git repository, including all the modules and their own
submodules.

```
git clone --recurse-submodules git@github.com:kineticsystem/stepit-workbench.git
cd stepit-workbench
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

Build the three images, then install the dependencies and compile the code of
every module inside its container. The first build takes a while: the robot's
image is based on ROS2 Jazzy desktop, and the editor's compiles
BehaviorTree.CPP.

```
./docker/dock.sh build
```

This is also how you pick up a change to a `Dockerfile` or to the code: it
rebuilds only the image layers that changed. To build a single module, name its
service: `stepit`, `commander` or `editor`.

```
./docker/dock.sh build commander
```

## Running the Application

Start the robot, the commander and the editor in the background:

```
./docker/dock.sh start
```

RViz opens with the robot, and the editor is on <http://localhost:8080>. Open
an objective, e.g. `OffsetJointsBy`, and press **Run** to execute it on the
robot.

Follow the output of every service, or of one of them. Stop following with
`Ctrl+C`: the services keep running.

```
./docker/dock.sh logs
./docker/dock.sh logs commander
```

Show which containers are running:

```
./docker/dock.sh status
```

Open a terminal into a container, e.g. to send an objective from the command
line:

```
./docker/dock.sh shell commander
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

To publish the editor on another port, set `EDITOR_PORT` when starting it, e.g.
`EDITOR_PORT=9000 ./docker/dock.sh start`.

## Working on a Module

Each module is a complete git repository of its own under [`modules`](modules):
edit, commit and push it as usual. Its README tells how to build and test it.

Inside a container, opened with `./docker/dock.sh shell <service>`, the module is
mounted at `~/ws`, and its scripts are on the `PATH` and aliased as in the
module's own container: `update`, `build`, `test`, and for the editor `serve`,
`dev` and `validate`.

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

The workbench records the commit of each module it was tested with. To move
every module to the latest commit of its `main` branch:

```
git submodule update --remote --recursive
./docker/dock.sh build
```

Then commit the new commits of the modules in the workbench:

```
git add modules
git commit -m "Update the modules"
```

## How It Works

[`docker/docker-compose.yml`](docker/docker-compose.yml) defines one service per
module, each extending the `dev` service of the module's own
`docker/docker-compose.yml`. The Dockerfiles, the mounts and the network
settings therefore stay defined in one place, the modules, and the workbench
only overrides:

- the names of the containers and images, prefixed with `workbench-`, so they
  do not clash with the containers each module creates with its own
  `dock.sh`;
- the command, which runs the application instead of keeping an idle
  container: `ros2 launch robot_bringup launch.py`,
  `ros2 launch stepit_server commander.launch.py` and `serve.sh`;
- the folder the editor opens: the commander's
  `src/stepit_objectives/objectives`, instead of the editor's examples.

The robot and the commander use the host network, so they discover each other
over DDS. The editor reaches the commander from the browser, through rosbridge
on `ws://localhost:9090`.
