# FocusStack

[`focus_stack.xml`](../src/plugins/stepit_objectives/objectives/focus_stack.xml) shoots a focus stack at each of several angles of the rotary stage. At each angle, the rail steps the camera from the near end to the far end of the subject, marked beforehand with [`MarkNear`](MarkNear.md) and [`MarkFar`](MarkFar.md), and a shot is taken at every step. With 10 shots and 35 angles, that is 350 pictures, which a stacking program then turns into one sharp picture per angle, and a 3D model.

| Parameter | Description |
|---|---|
| `shots` | How many shots from the near end of the rail to the far one, both included, e.g. `10`. |
| `stage_from` | The first angle of the stage, in radians of its motor, from where the stage is, e.g. `-1.0`. |
| `stage_to` | The last angle, e.g. `1.0`. Equal to `stage_from` for a single stack, without turning. |
| `angles` | How many angles from the first to the last, both included, e.g. `35`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: FocusStack, payload: '{shots: 10, stage_from: -1.0, stage_to: 1.0, angles: 35}'}"
```

```
FocusStack
├── SubTree EnsureControllers       (position_controller only)
├── GetJointPositions               -> {home}: the angles are from here, and the joints come back here
├── LoadValues  near, far           (the marks, from the state file)
├── SetJoints                       -> {first}, {last}: the first and the last position of the stack
├── CommandJointPositions           to {first}, past it and back
├── Steps  stage_from to stage_to, `angles` values
│   └── Steps  near to far, `shots` values
│       ├── CommandJointPositions   to the stage's angle and the rail's step, from {first} toward {last}
│       ├── Sleep                   500 ms, for the vibrations to die down
│       └── RetryUntilSuccessful    2 attempts
│           └── ExpectPicture       (fails if no picture comes within 15 s)
│               └── Shoot           (the Freezer's default sequence: the camera and the lights)
└── CommandJointPositions           back to {home}
```

Every move goes through the position controller, as in [`Stack`](Stack.md): the microcontroller plans each one on its own profile, and `CommandJointPositions` waits until the joints have arrived and stopped before the shot.

**The angles are motor radians, for now.** Degrees of the stage need the gear ratio between its motor and the stage, which is not measured yet, see [`TODO.md`](../TODO.md). The same goes for millimetres of the rail; the rail needs none to run a stack, as its two ends are marked where they are.

**Every position is approached from the same side, against backlash.** Each move approaches its position going from the first position of the stack to the last: the rail from near to far, the stage from `stage_from` to `stage_to`. A joint that would arrive the other way first goes past its target, then back to it, so that its gears always end loaded the same way. That happens when the rail comes back to the near end for the next angle, and when the stage turns to its first angle against the direction of the stack. The first move also sends a joint that is already at its first position past it and back, as it may have been driven there either way. How far past is the overshoot of each motor, `overshoot.joint1` and `overshoot.joint2` in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml): it must exceed the backlash of the axis.

**A shot without a picture is fired again, once.** The Freezer fires the camera through a wire, and cannot tell whether the shutter opened: the camera sometimes ignores the release. `ExpectPicture` waits for StepIt Camera to report the picture on `/camera/picture`; when none comes, the shot is fired again, and after a second miss the stack stops, rather than leave a gap. The log names the shot.

**The marks are counts of motor steps.** The controller of the motors counts from 0 when it powers up, so marks saved before it restarts point somewhere else after: mark both ends again for every subject.

It fails when a mark is missing, saying which, before anything moves. Cancelling it stops the robot where it is, without going back.
