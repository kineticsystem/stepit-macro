# StepIt UI

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [The Page](#the-page)
- [Running the Application](#running-the-application)
- [Working on the Page](#working-on-the-page)
- [How It Works](#how-it-works)
  - [The Layers](#the-layers)
  - [Tasks and Configuration](#tasks-and-configuration)
  - [The Sliders](#the-sliders)
  - [The Pictures](#the-pictures)
- [Tests](#tests)
- [Design Decisions and Trade-offs](#design-decisions-and-trade-offs)

## Introduction

StepIt UI is the application of [StepIt Macro](../README.md), an automated macro photography rig for 3D focus stacking: a camera on a motorized rail, on a rotary stage, with lights. It puts the whole rig on one web page, made for a tablet with a touch screen on the rig's network, and usable from a desktop too.

- See what the camera sees, live, and set the ISO, the shutter speed, the aperture, the white balance and the exposure compensation.
- Take a test shot, fired by the Freezer board with the lights, and see its picture.
- Switch the lights on and off.
- Drive the rotary stage and the rail by hand, with two vertical sliders under the thumbs, which work like the sticks of a gamepad.
- Mark the two ends of a focus stack on the rail, and shoot the stack at every angle of the stage, watching its progress and its pictures.
- Stop whatever the rig does, from a button always at the end of the toolbar.

Every page shows the same, whichever device opened it, and when: what runs, the marks and the counts of a stack, its progress and its pictures come from the rig, not from the page that started them.

It is a React page, built with Vite and TypeScript, with pnpm. It needs no ROS in the browser: it talks to the rig through [rosbridge](https://github.com/RobotWebTools/rosbridge_suite), a WebSocket that speaks JSON.

## The Page

| Part | What it does | Through |
|---|---|---|
| Top bar | Whether the page reaches the rig, which task runs, whoever started it, and why the last task of this page failed. | the commander |
| **Live view** | The camera's live view, in the middle, started and stopped by a button of the toolbar above it, green while the stream runs. When it is off, the middle shows the last picture the camera took, whoever fired it. The messages of the shot and of the lights show over the bottom. | the camera's driver, and web_video_server |
| **Test shot** | Stops the live view, then runs the objective `TakeShot`: StepIt Freezer fires the camera through its jack, with the lights, and the picture goes into the folder `tests` of the pictures. It takes the place of the live view as soon as the camera has downloaded it. | the commander, then the camera's driver |
| **Lights** | A button, green while the lights are on. It shows what the board does: a shot ends with every output off, lights included. | StepIt Freezer |
| **Manual drive** | On and off. On, it runs the objective `ActivateTeleop`, and the sliders drive the joints; the button is green. Pressed again, it releases the sliders and runs `ActivateController` with `joint_trajectory_controller`, the controller the rig starts with. | the commander |
| **Settings** | Under the gear, on three tabs: the camera's settings, with the values the camera accepts right now, which depend on its mode dial and its lens; the theme; the servers. | the camera's driver |
| **Mark** | Above and below the rail's slider: the ends of a stack, the camera away from the subject above, close to it below. **Hold** for 0.6 s, the button filling as it lasts, to run `MarkNear` or `MarkFar`, which save where the rail is in the rig's state file: a thumb brushing it at the end of a drag marks nothing. Green, **Marked**, once marked, on every page, with a flash at every new mark. Once both ends are marked, a **tap** runs `MoveRailToMark`, back to that end to check the focus, then manual drive again. A start of the rig forgets the marks. | the commander |
| **Stack** | Under the live view, with **Shots**, **Turn ±**, how far the stage turns either way, and **Angles**, which the rig keeps in its state file, `focus_stack` of `rig.yaml` until a page sets them: runs `FocusStack`, with the stage from −turn to +turn; with one angle, a single stack, the stage stays where it is and Turn is off. When Stack is off, the top bar says why, e.g. *Mark the near and the far end first*. While it runs, a progress bar fills as its pictures come, on every page, and each picture takes the place of the live view. | the commander, `/focus_stack/progress` |
| **Stop** | At the end of the toolbar, always in the same place: stops every task, whoever started it, and lets the sliders go. Red while a task runs, or while the page does not know yet whether one does. | the commander |
| Sliders | One at each edge, under each thumb, with an icon on the knob: the rotary stage (`joint1`) on the left, up to 0.75 turns/s, and the rail (`joint2`) on the right, up to 3 turns/s. Up is positive, as a gamepad's stick pushed up: it turns the stage clockwise, and moves the camera away from the subject. | `ui_teleop` |

While a task runs, the page locks the camera's settings and the lights, so that a shoot is not changed halfway through.

The page is made for a tablet held in both hands, sideways or upright: the sliders take the two edges, the live view the middle, and the toolbar above it wraps on several rows on a narrow screen. The settings menu, under the gear, chooses the theme and, for debugging, other servers than those of the computer that served the page.

## Running the Application

StepIt UI is part of StepIt Macro, in its folder `ui`: the rig's container builds it into `ui/dist` and serves it on port 8070, see the rig's [README](../README.md). From a tablet on the same network, open port 8070 of the rig's computer, e.g. `http://192.168.100.26:8070`.

The page reaches the rig on the computer that served it:

| Server | Port | For |
|---|---|---|
| The commander's rosbridge | 9090 | everything but the pictures and the live view |
| web_video_server | 8081 | the live view, as MJPEG |
| The camera's web server | 8090 | the pictures |

## Working on the Page

In the container of StepIt Macro, opened with `./docker/dock.sh shell`, the page is in `~/ws/ui`. The rig's `update`, `build` and `test` install, build and test it with the others, through the scripts of `bin/ui`; to work on it alone, install its packages once:

```
cd ~/ws/ui
pnpm install
```

Run the development server, with hot reload, on port 5176, next to the running rig:

```
pnpm dev
```

Type check, test and build it, as the `ui` job of the rig's CI does:

```
pnpm run typecheck
pnpm run test
pnpm run build
```

## How It Works

How the page is built, layer by layer, and a review of its structure, are in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

### The Layers

```
src/ros/         rosbridge.ts, the client of rosbridge; connection.ts, the one connection of the page
src/commander/   the objectives: run one, stop them all, and whether one runs
src/camera/      the camera driver's interface, its settings and live view, and the pictures
src/shot/        the shot, and the latest pictures
src/freezer/     the lights, as outputs of the Freezer board
src/motion/      the sliders: an axis per slider, sent as a gamepad
src/components/  React, one component per part of the page
```

Each folder keeps its state in a zustand store. The stores are kept up to date while the page is open by the `follow*()` functions, which `App.tsx` starts. The code that only computes, e.g. `motion/axis.ts`, `motion/joy.ts` and `freezer/outputs.ts`, knows nothing of the browser, so the tests run it as it is.

The client of rosbridge started as the one of StepIt Camera's test page, as did the camera's interface and the loading of the pictures; it also publishes topics, and sends action goals, the objectives:

```
→ call_service          a request                ← service_response
→ subscribe             a topic                  ← publish, for each message
→ advertise, publish    a topic, a message
→ send_action_goal      an objective             ← action_feedback, action_result
→ cancel_action_goal
```

It reconnects on its own, and subscribes and advertises again; a message published while it is disconnected is dropped, not sent late.

### Tasks and Configuration

**Only the robot's tasks go through the commander**, StepIt Commander, as objectives: a shot (`TakeShot`), handing the robot to the user (`ActivateTeleop`), marking and shooting a stack (`MarkNear`, `MarkFar`, `FocusStack`). The commander runs one objective at a time, and a new one replaces the one running, which is what a task should do: driving by hand stops a move.

**Configuring the rig goes straight to the drivers**: the camera's settings (`/camera/set_parameters`, `/camera/get_settings`), its live view (`/camera/start_streaming`, `/camera/stop_streaming`) and the lights (`/freezer/set_outputs`). Through the commander, a change of ISO would replace a running shoot.

The page knows whether an objective runs, whoever started it, from the status topic of the commander's action, `/commander/execute_objective/_action/status`, and which one from the commander's `/stepit_server/objective`. Both are latched: a page opened at any time gets them at once. A commander that just started publishes no status until its first goal, but publishes an empty objective at once: until a status comes, the objective tells whether one runs. Until either comes, Stop is on, red, since the page cannot tell that the robot is still.

**What the rig keeps, every page shows.** The page keeps nothing of the rig in the browser, only its own preferences, e.g. the theme. The marks of the rail and the counts of a stack are in the rig's state file, which the commander shows as its parameters `state.*`: the page reads them on every connection, follows them on `/parameter_events`, and sets the counts there, so that a mark or a count set on one device shows on every other. A running stack says how far it is on `/focus_stack/progress`, latched. **Stop** cancels every goal of the action, as `ros2 service call /commander/execute_objective/_action/cancel_goal action_msgs/srv/CancelGoal "{}"` does.

### The Sliders

The sliders never send velocities. The page sends them as a gamepad, a `sensor_msgs/Joy` on `/ui/joy` with one axis per slider, from -1 to 1, and `ui_teleop` in the rig, a second `gamepad_teleop`, turns the axes into velocities for the velocity controller, up to their speed at the ends: 0.75 turns/s for the rotary stage, which the subject turns on, and the motors' limit, 3 turns/s, for the rail. `ui_teleop`'s configuration, in the rig's `rig.yaml`, sets them: the page sends only where each knob is.

- While a slider is held away from its centre, the axes are sent 20 times a second, even when they do not change. When they stop coming for 0.5 s, e.g. when the tablet loses the network in the middle of a move, `ui_teleop` stops the joints. The velocity controller would otherwise keep the last velocity.
- Letting a slider go sends it once at rest, and then nothing: `ui_teleop` leaves the velocity controller to others, e.g. the gamepad.
- Each slider follows the thumb that grabbed it, so two thumbs drive both at once. The page stops both when it is hidden, e.g. when the tablet goes to sleep.
- A small zone around the centre is 0: a finger never rests exactly there.

The sliders work while the velocity controller runs, which the page reads from the controller manager every 2 seconds: any objective may change it, e.g. a move, which takes the trajectory controller instead. They are disabled while the page is disconnected from the rig, and come back when it reconnects, if the velocity controller still runs.

### The Pictures

We stream to frame the subject, and take pictures with the live view off: a shot stops it, and the last picture takes its place. A Canon EOS breaks its live view during a shot anyway.

The camera is fired by StepIt Freezer, through its jack, not over USB: the camera's driver sees the new file, downloads it, and tells of it on `/camera/picture`. Every page listens all the time, and shows every picture, whoever fired it. It loads it from the camera's web server at its `relative_path`, in the folder the rig chose (`tests` for a test shot, the stack's folder and its angle's for a stack), as StepIt Camera's test page does: a JPEG as it is, the preview inside a RAW with two `Range` requests. The server allows every origin, so the page, served from another port, can load the pictures. The page asks the size of a picture first, with a `HEAD` request, and never reads past its end: the server, cpp-httplib 0.14, answers a range past the end with a wrong `Content-Length`, which the browser drops.

The files stay on the rig, in the folder `pictures` of the repo: the page neither names nor downloads them.

## Tests

`pnpm run test` runs vitest on `tests`, with a fake WebSocket that answers as rosbridge:

| Test | What it covers |
|---|---|
| `rosbridge.test.ts` | Services, subscriptions, fragments, reconnection, publishing and action goals. |
| `commander.test.ts` | Running an objective, how it ended, stopping them all, whether one runs, and which. |
| `camera.test.ts` | The camera's settings, live view and pictures, their address in their folder, and how the settings are shown. |
| `picture.test.ts`, `raw.test.ts` | Loading a picture, and the preview inside a RAW. |
| `shot.test.ts` | The pictures kept, the latest two, and the previews of the others released. |
| `joy.test.ts` | The sliders as a gamepad: what is sent, and how often. |
| `axis.test.ts` | A slider as an axis. |
| `stack.test.ts` | The plan of a stack: the pictures it takes, the payload of `FocusStack`, what stops it from running, the defaults. |
| `hold.test.ts` | A press as a tap or a hold: early release, long press, abandoned, repeated keydown. |
| `stackParameters.test.ts` | The stack as the rig keeps it, in the commander's parameters: the marks, the counts, rig.yaml's angles. |
| `lights.test.ts` | The lights as outputs of the Freezer board. |

The page itself, its layout on a tablet and the touch of the sliders, has no automated test: check it by hand on the rig.

## Design Decisions and Trade-offs

**One page, not one per module.** The test pages of StepIt Camera and StepIt Freezer test their modules alone; this page runs the rig. In StepIt Macro, the test pages are turned off.

**One rosbridge, the commander's.** A rosbridge only knows the messages installed next to it, which is why each module's test page has its own. In StepIt Macro every module is installed in one container, so the commander's knows them all.

**Static files.** The page needs no server of its own: StepIt Macro serves the built files with Python's `http.server`, and the page talks to the rig through the servers that run anyway.

**No authentication.** Anyone on the network who reaches the rig's rosbridge can drive it. That suits a workshop's own network, not a shared one.
