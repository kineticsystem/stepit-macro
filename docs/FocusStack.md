# FocusStack

[`focus_stack.xml`](../src/plugins/stepit_objectives/objectives/focus_stack.xml) shoots a focus stack at each of several angles of the rotary stage. At each angle, the rail steps the camera from the near end to the far end of the subject, marked beforehand with [`MarkNear`](MarkNear.md) and [`MarkFar`](MarkFar.md), and a shot is taken at every step. With 10 shots and 35 angles, that is 350 pictures, which a stacking program then turns into one sharp picture per angle, and a 3D model.

| Parameter | Description |
|---|---|
| `shots` | How many shots from the near end of the rail to the far one, both included, e.g. `10`. |
| `stage_from` | The first angle of the stage, in degrees, from where the stage is, e.g. `-17`. |
| `stage_to` | The last angle, e.g. `17`. Equal to `stage_from` for a single stack, without turning. |
| `angles` | How many angles from the first to the last, both included, e.g. `35`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: FocusStack, payload: '{shots: 10, stage_from: -17, stage_to: 17, angles: 35}'}"
```

```
FocusStack
├── SubTree EnsureControllers       (position_controller only)
├── GetJointPositions               -> {home}: the angles are from here, and the joints come back here
├── LoadValues  near, far           (the marks, from stack_state)
├── CurrentTime                     -> {stack_folder}, e.g. 2026-10-06_15-20-04
├── ReportProgress                  0 of shots × angles, on /focus_stack/progress
├── DegreesToRadians                stage_from, stage_to, with deg_per_turn.joint1
├── SetJoints                       -> {first}, {last}: the first and the last position of the stack
├── CommandJointPositions           to {first}, past it and back
├── Fallback
│   ├── Sequence
│   │   ├── Steps  stage_from to stage_to, `angles` values, in degrees
│   │   │   ├── DegreesToRadians            this angle
│   │   │   ├── SetPictureFolder            {stack_folder}/angle_01_-17.0deg, and so on
│   │   │   ├── Steps  near to far, `shots` values
│   │   │   │   ├── CommandJointPositions   to the stage's angle and the rail's step, from {first} toward {last}
│   │   │   │   ├── Sleep                   500 ms, for the vibrations to die down
│   │   │   │   ├── ExpectPicture           (fails if no picture comes within 15 s: the whole stack stops)
│   │   │   │   │   └── Shoot               (the Freezer's default sequence: the camera and the lights)
│   │   │   │   └── ReportProgress          one more picture
│   │   │   └── StackDone                   stack.json in the angle's folder, and its name on /focus_stack/stack_done
│   │   └── AllStacksDone                   all_stacks.json in the stack's folder, and its name on /focus_stack/all_stacks_done
│   └── Sequence                            when the stack failed
│       ├── SetPictureFolder                "", the pictures folder itself again
│       └── AlwaysFailure
├── SetPictureFolder                "", the pictures folder itself again
└── CommandJointPositions           back to {home}
```

Every move goes through the position controller, as in [`Stack`](Stack.md): the microcontroller plans each one on its own profile, and `CommandJointPositions` waits until the joints have arrived and stopped before the shot.

**The angles are degrees of the stage.** `DegreesToRadians` turns them into radians of its motor with `deg_per_turn.joint1`, 4.5 degrees per motor turn, its 80:1 gear, in the section `stepit_server` of `rig.yaml`. The rail needs no ratio to run a stack, as its two ends are marked where they are.

**Every position is approached from the same side, against backlash.** Each move approaches its position going from the first position of the stack to the last: the rail from near to far, the stage from `stage_from` to `stage_to`. A joint that would arrive the other way first goes past its target, then back to it, so that its gears always end loaded the same way. That happens when the rail comes back to the near end for the next angle, and when the stage turns to its first angle against the direction of the stack. The first move also sends a joint that is already at its first position past it and back, as it may have been driven there either way. How far past is the overshoot of each motor, `overshoot.joint1` and `overshoot.joint2` in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml): it must exceed the backlash of the axis.

**A shot without a picture stops the whole stack.** The Freezer fires the camera through a wire, and cannot tell whether the shutter opened: the camera sometimes ignores the release. `ExpectPicture` waits for StepIt Camera to report the picture on `/camera/picture`; when none comes, the stack stops at once, without firing the shot again: a missed picture is a fault to look into, not a gap to fill. The joints stay where they are, as when the stack is cancelled. The log names the shot.

**Each stack has its folder of pictures, with one folder per angle.** Under the camera's pictures folder, the folder `pictures` of the repo, the stack's folder is named after when it started, and holds one folder per angle, numbered from 1, with its angle in degrees: the pictures of one rail's stack, to stack together.

```
pictures/2026-10-06_15-20-04/angle_01_-17.0deg/IMG_5460.CR2 ... IMG_5469.CR2
pictures/2026-10-06_15-20-04/angle_02_-16.0deg/IMG_5470.CR2 ...
```

`SetPictureFolder` sets the parameter `folder` of StepIt Camera, which saves the next pictures there; when the stack ends, the pictures go to the pictures folder itself again, also when it fails, e.g. on a shot whose picture does not come, so that a later picture, from the Freezer's remote trigger or the camera's own shutter, never lands among the stack's. A stack stopped with Stop, or replaced by another objective, leaves the folder set: the commander halts the tree, and no node runs on a halt. The next stack, or a test shot, sets its own.

**Each finished angle says so.** A folder of pictures that stops growing may only be waiting for its next shot: the files alone cannot tell a finished angle from one in progress. Once the last picture of an angle is saved, `StackDone` publishes the folder, relative to the pictures folder, e.g. `2026-10-06_15-20-04/angle_01_-17.0deg`, on `/focus_stack/stack_done` (`std_msgs/String`), latched, as `/focus_stack/progress` is, and writes `stack.json` into the folder. The camera reports a picture only once it is saved, and `ExpectPicture` waits for that report, so every picture of the angle is on disk when `StackDone` runs.

`stack.json` makes the folder say for itself that it is complete, also to whoever reads the pictures later, without following the topic. It is written to a temporary file, then renamed, so it is either whole or absent:

```json
{
  "folder": "2026-10-06_15-20-04/angle_01_-17.0deg",
  "shots": 10,
  "angle": 1,
  "degrees": -17,
  "finished": "2026-10-06T15:24:31",
  "files": [
    "IMG_5460.CR2",
    "IMG_5461.CR2"
  ]
}
```

`files` lists every file of the folder: one per shot, or two with RAW+JPEG. The commander finds the folder through `pictures_folder` in the section `stepit_server` of `rig.yaml`, which is the camera's `download_directory` through a YAML anchor: the commander and the camera run in the same container. An angle that stops halfway is never announced and gets no `stack.json`. When the file cannot be written, `StackDone` logs why and still announces the stack: the shooting goes on.

**The whole stack says so too, once every angle is done.** The stack's folder, like an angle's, cannot tell by itself whether more angles are coming. After the `StackDone` of the last angle, `AllStacksDone` publishes the stack's folder, e.g. `2026-10-06_15-20-04`, on `/focus_stack/all_stacks_done` (`std_msgs/String`), latched, and writes `all_stacks.json` into it, also written to a temporary file and renamed:

```json
{
  "folder": "2026-10-06_15-20-04",
  "shots": 10,
  "angles": 35,
  "finished": "2026-10-06T16:51:02",
  "stacks": [
    "angle_01_-17.0deg",
    "angle_02_-16.0deg"
  ]
}
```

Both topics exist from the moment the commander loads the plugin, not from the first stack, and serve every stack after it. A subscriber that comes through rosbridge, which runs in a process of its own, takes a moment to find a new publisher: a publisher made at the first `StackDone` of a stack would publish before it is found, and that message, the first angle, would be lost. A subscriber that finds the topics already there also subscribes latched, so it gets the last message again when it reconnects.

`stacks` lists the folders of the angles, each with its `stack.json`. A stack that stops halfway is never announced and gets no `all_stacks.json`: its finished angles keep theirs. When the file cannot be written, `AllStacksDone` logs why and still announces the stack.

**Every page shows how far it is.** `ReportProgress` publishes the pictures taken and the total, `done` and `total`, on `/focus_stack/progress` (`stepit_macro_msgs/StackProgress`), latched: a page opened on any device while the stack runs gets the current value at once, and StepIt UI shows it as a progress bar, whichever page started the stack.

**The marks are counts of motor steps.** The controller of the motors counts from 0 when it powers up, so marks saved before it restarts point somewhere else after: mark both ends again for every subject.

It fails when a mark is missing, saying which, before anything moves. Cancelling it stops the robot where it is, without going back.
