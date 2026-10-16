# StepIt UI Review

## Table of Contents <!-- omit in toc -->

- [Is the Page Well Structured?](#is-the-page-well-structured)
- [Are the Layers Respected?](#are-the-layers-respected)
- [SOLID](#solid)
- [Coupling Between Components](#coupling-between-components)
- [Duplication](#duplication)
- [Implementation Issues](#implementation-issues)
- [Recommendations](#recommendations)

Reviewed on 2026-10-09, at commit `cfe83b7` of StepIt Macro, with the `architect` skill. It judges the design that [ARCHITECTURE.md](ARCHITECTURE.md) describes; the rest of the rig is described in the repo's [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md). Each finding names where it is; the ones marked *verified* were reproduced with a throwaway vitest test in the rig's image, the others come from reading the code.

## Is the Page Well Structured?

Yes, for its size: about 2,900 lines of TypeScript, one folder per device of the rig, the protocol in one class, and the pure logic in small functions with tests of their own: the axes, the joy messages, the bits of a jack, the formats of the settings, the RAW previews, the plan of a stack, the stack's parameters, the hold of a button. The rule of the rig, tasks through the commander and configuration straight to the drivers, is followed everywhere, and the safety of the sliders rests on `ui_teleop`'s watchdog, not on the page, as it should. Nothing of the rig is kept in the browser: the marks and the counts live on the rig, and every page shows the same.

The weak part is the layer of state. It holds the code that orchestrates, the shot, manual drive, the stack, the reconnections, and it is the one layer with no seam to test it and no rule about what it may know. Both verified bugs below are in it.

## Are the Layers Respected?

Mostly. Three kinds of shortcut:

1. **Stores that talk ROS themselves.** The commander, the camera and `power_off` have an interface that names their services and topics; the others do not. [`freezer/lights.ts`](../src/freezer/lights.ts) calls `/freezer/set_outputs` and subscribes to `/freezer/outputs`; [`motion/store.ts`](../src/motion/store.ts) calls `/controller_manager/list_controllers`; and [`stack/store.ts`](../src/stack/store.ts), the largest, calls the commander's `set_parameters`, `list_parameters` and `get_parameters` and follows `/parameter_events` and `/focus_stack/progress`, each with the types of the messages declared in the store. The rig keeps the names of robot interfaces in one package, `stepit_behaviors`; the page has no such rule.
2. **Views that know the rig's objectives.** `ManualDriveButton` disables itself while `running` is `ActivateTeleop` or `ActivateController` ([`Toolbar.tsx`](../src/components/Toolbar.tsx)), the names of `useMotion`'s objectives; `StackBar` shows the progress while `objective` is `FocusStack` ([`StackBar.tsx`](../src/components/StackBar.tsx)), a name `useStack` already uses.
3. **Views and stores that import the transport for a helper.** `errorMessage()` lives in `rosbridge.ts`, so `LiveView`, which has nothing to do with rosbridge, imports the transport for it, as does every store.

The rules of what may be done when, connected, no task running, are also in the views: see [Coupling Between Components](#coupling-between-components).

## SOLID

| Principle | Verdict |
|---|---|
| **Single responsibility** | Good in the transport and the interfaces. `Rosbridge` is long, 320 lines, but about one thing, the protocol. `settings.ts` has three jobs: the preferences, the URLs of the servers, and the theme of the document, applied at import time. `useStack` holds the plan, the marks, the progress and the commander's parameters protocol. `Toolbar` holds five buttons, but as five small components. |
| **Open/closed** | Good where it matters: a new camera setting needs no code, a new objective is a string, a new module is a new folder. Adding a rule such as "locked during a task" means editing each button that applies it. |
| **Liskov substitution** | Barely applies: there is no inheritance beyond the error classes. |
| **Interface segregation** | Weak in the views: `useCamera()`, `useShot()`, `useCommander()`, `useMotion()`, `useStack()` and `usePower()` are read whole, without a selector, so a component re-renders on any change of the store, e.g. `LiveViewButton` and `LiveView` every 3 s when the settings are read again, as the poll always stores a new array, and `TaskStatus` on every change of the stack. Harmless at this size. |
| **Dependency inversion** | Good in the layer of interfaces: `Camera`, `powerOff()` and the commander's functions take a `Rosbridge`, `JoyPublisher` takes a function to publish with, which is why they are tested. Broken in the layer of state: every store reaches the concrete singleton `ros()` and the other stores directly, which is why none is tested. |

## Coupling Between Components

The stores form a clean, acyclic graph (see [The Stores](ARCHITECTURE.md#the-stores)). The coupling is in the views:

- **The rules of when a command may run are spread over the buttons, and two buttons have none.** `connected` is computed nine times, in six files, and `busy` read in seven components, each combining them its own way: `ShotButton` (`!connected || busy || shooting`), `LightsButton` (`!connected || !known || busy || switching`), `StackBar` (`!idle || problem`), `RailMark` (`!connected || busy || marking`), `CameraSettings` (`busy`), `PowerButton` (through `powerProblem()`). `ManualDriveButton` checks two objective names and not `busy`, `LiveViewButton` checks nothing: see [Implementation Issues](#implementation-issues). None of it is tested.
- **`Toolbar` reads every device store**, and `LiveView` reads four, one of them only to show the lights' error. The errors of the page have seven shapes in seven places: `error` and `streamError` in `useCamera`, `refused` per setting, `failure` in `useCommander` and in `usePower`, `message` with `state` in `useShot`, `error` in `useLights` and in `useStack`. Each view picks which to show.
- **`useShot` and `useStack` drive `useCamera`**: both stop the live view before they shoot. It is the right place, a shot and a stack need both, but the rule "no live view while shooting" is in two stores and in no button.
- **The page repeats values of the rig.** `SLIDERS` and `App.tsx` know that `joint1` is the stage and `joint2` the rail; `STATE` in the stack store is the node name of `stack_state`.

Each component can still be changed alone; what cannot is a rule that spans them.

## Duplication

| What | Where | Weight |
|---|---|---|
| **The rosbridge client and the camera's code, copied across modules.** | `ros/rosbridge.ts` exists in StepIt Camera's test page, StepIt Freezer's board page and here, each different (121 and 370 lines of `diff` against this one). `camera/format.ts` is an identical copy; `camera/raw.ts` differs by one function, `camera/camera.ts` by 34 lines, `camera/picture.ts` by 21. | High: the fixes do not travel. This page reads the size of a picture first, to step around cpp-httplib 0.14; the camera's test page still reads 64 KB from the start of every file, the case that fails for a small JPEG. |
| Following a topic across connections: subscribe, then again on every `onConnected`. | `followCommander`, `followLights`, `followPictures`, `followDevices`; `followStack` does not. | High: one store forgot it, see issue 2, and the others overlap the client's own re-subscription, see issue 3. |
| The QoS of a latched topic, `reliable`, `transient_local`, `keep_last`, 1. | Twice in [`commander.ts`](../src/commander/commander.ts), in [`lights.ts`](../src/freezer/lights.ts), [`stack/store.ts`](../src/stack/store.ts) and [`devices.ts`](../src/devices/devices.ts). | Low. |
| `connected`, `busy` in the views. | Six files, see above. | Medium. |
| The values of the rig. | `SLIDERS`, `STATE`, see above. | Medium: kept in step with `rig.yaml` by hand. |
| The theme: its storage key and how `auto` resolves. | [`index.html`](../index.html), before the first paint, and [`settings.ts`](../src/settings.ts). | Low, and deliberate. |
| A press told from a hold, wired to a button. | `RailMark` and `PowerButton`: the same eight pointer and key handlers around `createHold()`. | Low. |
| `Cannot load … : status statusText`. | `fetchSize`, `fetchRange` in [`picture.ts`](../src/camera/picture.ts). | Low. |

## Implementation Issues

Ordered from the most to the least serious. None of them is a safety issue: the sliders stop through `ui_teleop`'s watchdog whatever the page does, and the commander's preemption is a controlled stop.

1. **The stores cannot be tested** (*verified*). [`settings.ts`](../src/settings.ts) calls `window.matchMedia` at import time, to apply the theme, so importing any store in Node.js fails with `ReferenceError: window is not defined`; every store imports it through `connection.ts`. With the global `ros()` and the stores created at import, a test also needs `vi.resetModules()` between cases. The untested code is the one that orchestrates: `takeShot`, `enable` and `disable`, `mark`, `start`, the `follow*()` functions.
2. **The stack stops following the rig after a change of rosbridge** (*verified*). `followStack()` subscribes to `/parameter_events` and `/focus_stack/progress` once, on the `Rosbridge` of the moment; when the settings name another rosbridge, `ros()` closes that client and opens a new one, which never subscribes to them. With the stores stubbed into Node.js, the new connection received no `subscribe` for either topic. From then on, a mark set from another page, a count changed elsewhere and the progress of a running stack do not show until the page is reloaded; `readStack()` on each connection still reads the values once.
3. **Two layers re-subscribe after a connection** (*verified*). `Rosbridge`'s `onopen` sets the status to connected, which runs the `onConnected` listeners, before it sends the subscriptions again. `followCommander`, `followLights` and `followPictures` then unsubscribe and subscribe again, and `onopen` sends that new subscription a second time. On the first connection, rosbridge receives an `unsubscribe` for an id it never saw, then the same `subscribe` twice. rosbridge copes, but the page does not own its subscriptions in one place, which is how issue 2 came about.
4. **Manual drive and Live view are not locked while a task runs.** `ManualDriveButton` is off only while its own objectives run, so a tap during a focus stack runs `ActivateTeleop`, which the commander lets preempt the stack: 350 pictures end on a tap next to the sliders, while Stack, Mark, Test shot, Lights and the camera's settings are all locked during a task. `LiveViewButton` checks nothing, so the live view can be started in the middle of a stack, which `start()` turned it off for.
5. **The live view is the page's belief.** `setStreaming()` in [`camera/store.ts`](../src/camera/store.ts) sets `streaming` before the call, and keeps it when the call fails: a failed stop leaves the camera streaming while the button says it is off. The driver publishes no state of its stream to correct it.
6. **The counts of a stack are written to the rig at every keystroke, and accept what the plan refuses.** `NumberField` in [`StackBar.tsx`](../src/components/StackBar.tsx) calls `setPlan()` for every valid value typed, each a `set_parameters` that writes the rig's state file; typing 350 writes 3, 35 and 350. Shots and Angles have no `min`, so 0 or -5 is saved on the rig, for every page, and only then reported by `planProblem()`. The comment of `StackBar` also names a `StageMark` component, which does not exist.

## Recommendations

In order, each one small enough for one pull request:

1. **Make the stores testable** (fixes issue 1). Move the theme out of `settings.ts` into a `theme.ts` that `main.tsx` starts, so that importing the settings has no effect on the document. Then test a change of rosbridge, `takeShot`, manual drive and a stack against `FakeSocket`, with `vi.resetModules()` between cases. Later, if the stores grow, create them with a factory that takes the connection.
2. **Give the connection one owner of the subscriptions** (fixes issues 2 and 3). Let `connection.ts` offer `follow(topic, type, listener, qos)`, which keeps the subscription across a change of rosbridge by moving it to the new `Rosbridge`, and does nothing on a mere reconnection, which the client already handles. `followCommander`, `followLights`, `followPictures` and `followStack` then shrink to calls of it, and the duplicate traffic goes.
3. **Move the rules of the buttons into the stores** (fixes issue 4). Selectors such as `canTakeShot`, `canDrive`, `canStream`, one rule each, tested once recommendation 1 is done; a `switching` flag in `useMotion` in place of the objective names in `ManualDriveButton`; Manual drive and Live view locked while a task runs, Stop being the one way to end it. If taking the robot by hand mid-task is wanted, say so in the button, and keep it apart from the others. Read the stores with selectors, or `useShallow`, rather than whole.
4. **Give every module an interface.** `freezer/freezer.ts` with `setOutputs()` and `onOutputs()`, `motion/controllers.ts` with `activeControllers()`, and the commander's parameters, `getParameters()`, `setParameters()`, `onParameters()`, next to `runObjective()`, as `Camera` does; a `LATCHED` QoS constant in `rosbridge.ts`. Then only the interfaces name ROS topics and services, the page's version of the rig's own rule. Move `errorMessage()` to a `util.ts`.
5. **Apply the counts when they are done, not as they are typed** (fixes issue 6). On blur and Enter, as `UrlField` does, with a `min` of 1 for Shots and Angles; correct the comment of `StackBar`.
6. **Follow the live view's real state** (fixes issue 5). At the least, read the state back after a failed call; better, have StepIt Camera publish whether it streams, latched, and follow it as the lights are followed. The second is a change to StepIt Camera.
7. **Share the code copied across modules.** One small package, e.g. `stepit-web`, with the rosbridge client and the camera's interface, used by the three pages; or, at the least, port this page's reading of the picture's size to StepIt Camera's test page. Sharing costs each module its independence at build time, which is why the code is copied; three diverging copies of a protocol client is the point where that trade stops paying.
