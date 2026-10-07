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
├── LoadValues  near, far           (the marks, from the state file)
├── CurrentTime                     -> {stack_folder}, e.g. 2026-10-06_15-20-04
├── ReportProgress                  0 of shots × angles, on /focus_stack/progress
├── DegreesToRadians                stage_from, stage_to, with deg_per_turn.joint1
├── SetJoints                       -> {first}, {last}: the first and the last position of the stack
├── CommandJointPositions           to {first}, past it and back
├── Steps  stage_from to stage_to, `angles` values, in degrees
│   ├── DegreesToRadians            this angle
│   ├── SetPictureFolder            {stack_folder}/angle_01_-17.0deg, and so on
│   └── Steps  near to far, `shots` values
│       ├── CommandJointPositions   to the stage's angle and the rail's step, from {first} toward {last}
│       ├── Sleep                   500 ms, for the vibrations to die down
│       └── RetryUntilSuccessful    2 attempts
│           └── ExpectPicture       (fails if no picture comes within 15 s)
│               └── Shoot           (the Freezer's default sequence: the camera and the lights)
│           └── ReportProgress      one more picture
│   └── StackDone                   stack.json in the angle's folder, and its name on /focus_stack/stack_done
├── SetPictureFolder                "", the pictures folder itself again
└── CommandJointPositions           back to {home}
```

Every move goes through the position controller, as in [`Stack`](Stack.md): the microcontroller plans each one on its own profile, and `CommandJointPositions` waits until the joints have arrived and stopped before the shot.

**The angles are degrees of the stage.** `DegreesToRadians` turns them into radians of its motor with `deg_per_turn.joint1`, 4.5 degrees per motor turn, its 80:1 gear, in the section `stepit_server` of `rig.yaml`. The rail needs no ratio to run a stack, as its two ends are marked where they are.

**Every position is approached from the same side, against backlash.** Each move approaches its position going from the first position of the stack to the last: the rail from near to far, the stage from `stage_from` to `stage_to`. A joint that would arrive the other way first goes past its target, then back to it, so that its gears always end loaded the same way. That happens when the rail comes back to the near end for the next angle, and when the stage turns to its first angle against the direction of the stack. The first move also sends a joint that is already at its first position past it and back, as it may have been driven there either way. How far past is the overshoot of each motor, `overshoot.joint1` and `overshoot.joint2` in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml): it must exceed the backlash of the axis.

**A shot without a picture is fired again, once.** The Freezer fires the camera through a wire, and cannot tell whether the shutter opened: the camera sometimes ignores the release. `ExpectPicture` waits for StepIt Camera to report the picture on `/camera/picture`; when none comes, the shot is fired again, and after a second miss the stack stops, rather than leave a gap. The log names the shot.

**Each stack has its folder of pictures, with one folder per angle.** Under the camera's pictures folder, the folder `pictures` of the repo, the stack's folder is named after when it started, and holds one folder per angle, numbered from 1, with its angle in degrees: the pictures of one rail's stack, to stack together.

```
pictures/2026-10-06_15-20-04/angle_01_-17.0deg/IMG_5460.CR2 ... IMG_5469.CR2
pictures/2026-10-06_15-20-04/angle_02_-16.0deg/IMG_5470.CR2 ...
```

`SetPictureFolder` sets the parameter `folder` of StepIt Camera, which saves the next pictures there; when the stack ends, the pictures go to the pictures folder itself again. A stack that stops halfway leaves the folder set: the next stack, or a test shot, sets its own.

**Each finished angle is announced, for a stacking program.** Once the last picture of an angle is saved, `StackDone` writes `stack.json` into the angle's folder and publishes the folder, relative to the pictures folder, e.g. `2026-10-06_15-20-04/angle_01_-17.0deg`, on `/focus_stack/stack_done` (`std_msgs/String`), latched. A stacking program on another computer listens through the commander's rosbridge, port 9090, copies that folder and merges it while the rig shoots the next angle. The camera reports a picture only once it is saved, and `ExpectPicture` waits for that report, so every picture of the angle is on disk when `StackDone` runs.

`stack.json` makes the folder say for itself that it is complete, for a program that was not listening, e.g. a computer switched off during the stack: it finds the finished stacks by it. It is written to a temporary file, then renamed, so it is either whole or absent:

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

**Every page shows how far it is.** `ReportProgress` publishes the pictures taken and the total, `[done, total]`, on `/focus_stack/progress` (`std_msgs/Int32MultiArray`), latched: a page opened on any device while the stack runs gets the current value at once, and StepIt UI shows it as a progress bar, whichever page started the stack.

**The marks are counts of motor steps.** The controller of the motors counts from 0 when it powers up, so marks saved before it restarts point somewhere else after: mark both ends again for every subject.

It fails when a mark is missing, saying which, before anything moves. Cancelling it stops the robot where it is, without going back.
