# StepIt UI

[![CI](https://github.com/kineticsystem/stepit-ui/actions/workflows/ci.yml/badge.svg)](https://github.com/kineticsystem/stepit-ui/actions/workflows/ci.yml)

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

StepIt UI is the application of [StepIt Macro](https://github.com/kineticsystem/stepit-macro), an automated macro photography rig for 3D focus stacking: a camera on a motorized rail, on a rotary stage, with lights. It puts the whole rig on one web page, made for a tablet with a touch screen on the rig's network, and usable from a desktop too.

- See what the camera sees, live, and set the ISO, the shutter speed, the aperture, the white balance and the exposure compensation.
- Take a shot, fired by the Freezer board with the lights, and see and download its pictures.
- Switch the lights on and off.
- Drive the rotary stage and the rail by hand, with two vertical sliders under the thumbs, which work like the sticks of a gamepad.
- Stop whatever the rig does, from a button always in the top bar.

It is a React page, built with Vite and TypeScript, with pnpm. It needs no ROS in the browser: it talks to the rig through [rosbridge](https://github.com/RobotWebTools/rosbridge_suite), a WebSocket that speaks JSON.

## The Page

| Part | What it does | Through |
|---|---|---|
| Top bar | Whether the page reaches the rig, which task runs, and **Stop**, which stops every task, whoever started it, and the sliders. | the commander |
| **Live view** | The camera's live view, in the middle, started and stopped by a button of the toolbar above it, green while the stream runs. When it is off, the middle shows the last photo, with its name and **Download**. The messages of the shot and of the lights show over the bottom. | the camera's driver, and web_video_server |
| **Take a shot** | Stops the live view, then runs the objective `TakeShot`: StepIt Freezer fires the camera through its jack, with the lights. The picture takes the place of the live view as soon as the camera has downloaded it. | the commander, then the camera's driver |
| **Lights** | A button, green while the lights are on. It shows what the board does: a shot ends with every output off, lights included. | StepIt Freezer |
| **Manual drive** | Runs the objective `ActivateTeleop`; the sliders then drive the joints. The button stays, green while they do. | the commander |
| **Camera** | A panel of the camera's settings, with the values the camera accepts right now, which depend on its mode dial and its lens. | the camera's driver |
| Sliders | One at each edge, under each thumb: the rotary stage (`joint1`) on the left, up to 1.5 turns/s, and the rail (`joint2`) on the right, up to 3 turns/s. Up is positive, as a gamepad's stick pushed up. | `ui_teleop` |

While a task runs, the page locks the camera's settings and the lights, so that a shoot is not changed halfway through.

The page is made for a tablet held in both hands, sideways or upright: the sliders take the two edges, the live view the middle, and the toolbar above it wraps on several rows on a narrow screen. The settings menu, under the gear, chooses the theme and, for debugging, other servers than those of the computer that served the page.

## Running the Application

StepIt UI runs in the container of StepIt Macro, which builds it into `dist` and serves it on port 8070: see its README. From a tablet on the same network, open port 8070 of the rig's computer, e.g. `http://192.168.100.26:8070`.

The page reaches the rig on the computer that served it:

| Server | Port | For |
|---|---|---|
| The commander's rosbridge | 9090 | everything but the pictures and the live view |
| web_video_server | 8081 | the live view, as MJPEG |
| The camera's web server | 8090 | the pictures |

## Working on the Page

In the container of StepIt Macro, opened with `./docker/dock.sh shell`, the page is in `~/ws/modules/stepit-ui`. Install its packages once:

```
cd ~/ws/modules/stepit-ui
pnpm install
```

Run the development server, with hot reload, on port 5176, next to the running rig:

```
pnpm dev
```

Type check, test and build it, as CI does:

```
pnpm run typecheck
pnpm run test
pnpm run build
```

## How It Works

### The Layers

```
src/ros/         rosbridge.ts, the client of rosbridge; connection.ts, the one connection of the page
src/commander/   the objectives: run one, stop them all, and whether one runs
src/camera/      the camera driver's interface, its settings and live view, and the pictures
src/shot/        the shot, and the pictures of the session
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

**Only the robot's tasks go through the commander**, StepIt Commander, as objectives: a shot (`TakeShot`), and handing the robot to the user (`ActivateTeleop`). The commander runs one objective at a time, and a new one replaces the one running, which is what a task should do: driving by hand stops a move.

**Configuring the rig goes straight to the drivers**: the camera's settings (`/camera/set_parameters`, `/camera/get_settings`), its live view (`/camera/start_streaming`, `/camera/stop_streaming`) and the lights (`/freezer/set_outputs`). Through the commander, a change of ISO would replace a running shoot.

The page knows whether an objective runs, whoever started it, from the status topic of the commander's action, `/commander/execute_objective/_action/status`. **Stop** cancels every goal of the action, as `ros2 service call /commander/execute_objective/_action/cancel_goal action_msgs/srv/CancelGoal "{}"` does.

### The Sliders

The sliders never send velocities. The page sends them as a gamepad, a `sensor_msgs/Joy` on `/ui/joy` with one axis per slider, from -1 to 1, and `ui_teleop` in the rig, a second `gamepad_teleop`, turns the axes into velocities for the velocity controller, up to their speed at the ends: 1.5 turns/s for the rotary stage, which the subject turns on, and the motors' limit, 3 turns/s, for the rail. The page only labels the sliders with these speeds (`SLIDERS` in `motion/store.ts`); `ui_teleop`'s configuration, in the rig's `rig.yaml`, sets them.

- While a slider is held away from its centre, the axes are sent 20 times a second, even when they do not change. When they stop coming for 0.5 s, e.g. when the tablet loses the network in the middle of a move, `ui_teleop` stops the joints. The velocity controller would otherwise keep the last velocity.
- Letting a slider go sends it once at rest, and then nothing: `ui_teleop` leaves the velocity controller to others, e.g. the gamepad.
- Each slider follows the thumb that grabbed it, so two thumbs drive both at once. The page stops both when it is hidden, e.g. when the tablet goes to sleep.
- A small zone around the centre is 0: a finger never rests exactly there.

The sliders work while the velocity controller runs, which the page reads from the controller manager every 2 seconds: any objective may change it, e.g. a move, which takes the trajectory controller instead.

### The Pictures

We stream to frame the subject, and take pictures with the live view off: a shot stops it, and the last picture takes its place. A Canon EOS breaks its live view during a shot anyway.

The camera is fired by StepIt Freezer, through its jack, not over USB: the camera's driver sees the new file, downloads it, and tells of it on `/camera/picture`. The page listens before the shot, so that the picture cannot come first, and loads it from the camera's web server, as StepIt Camera's test page does: a JPEG as it is, the preview inside a RAW with two `Range` requests. The server allows every origin, so the page, served from another port, can load the pictures and download them: the `download` attribute of a link is ignored across origins, so **Download** fetches the file first. The page asks the size of a picture first, with a `HEAD` request, and never reads past its end: the server, cpp-httplib 0.14, answers a range past the end with a wrong `Content-Length`, which the browser drops.

## Tests

`pnpm run test` runs vitest on `tests`, with a fake WebSocket that answers as rosbridge:

| Test | What it covers |
|---|---|
| `rosbridge.test.ts` | Services, subscriptions, fragments, reconnection, publishing and action goals. |
| `commander.test.ts` | Running an objective, how it ended, stopping them all, and whether one runs. |
| `camera.test.ts` | The camera's settings, live view and pictures, and how the settings are shown. |
| `picture.test.ts`, `raw.test.ts` | Loading a picture, and the preview inside a RAW. |
| `joy.test.ts` | The sliders as a gamepad: what is sent, and how often. |
| `axis.test.ts` | A slider as an axis, and the speed it asks for. |
| `lights.test.ts` | The lights as outputs of the Freezer board. |

The page itself, its layout on a tablet and the touch of the sliders, has no automated test: check it by hand on the rig.

## Design Decisions and Trade-offs

**One page, not one per module.** The test pages of StepIt Camera and StepIt Freezer test their modules alone; this page runs the rig. In StepIt Macro, the test pages are turned off.

**One rosbridge, the commander's.** A rosbridge only knows the messages installed next to it, which is why each module's test page has its own. In StepIt Macro every module is installed in one container, so the commander's knows them all.

**Static files.** The page needs no server of its own: StepIt Macro serves the built files with Python's `http.server`, and the page talks to the rig through the servers that run anyway.

**No authentication.** Anyone on the network who reaches the rig's rosbridge can drive it. That suits a workshop's own network, not a shared one.
