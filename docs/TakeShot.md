# TakeShot

[`take_shot.xml`](../src/plugins/stepit_objectives/objectives/take_shot.xml) fires a shot on the StepIt Freezer board: the cameras, the flashes and the lights of a sequence, each line switched by the hardware timer of the board's Arduino Nano. The cameras are fired through their jacks, not over USB: StepIt Camera then downloads each new picture and publishes it on `/camera/picture`.

| Parameter | Default | Description |
|---|---|---|
| `sequence` | the Freezer's `default_sequence` | The sequence to fire, as named in the parameters of the Freezer node, in [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml), e.g. `test_shot`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: TakeShot, payload: '{sequence: test_shot}'}"
```

```
TakeShot
└── Sequence
    ├── SetPictureFolder   tests   (where the camera saves the picture)
    ├── Fallback
    │   ├── ExpectPicture   (fails if the camera reports no picture of the shot)
    │   │   └── Shoot   (sends the goal to /freezer/shoot, and waits for the end of the shot)
    │   └── Sequence   (when the shot failed)
    │       ├── SetPictureFolder   ""   (the pictures folder itself again)
    │       └── AlwaysFailure
    └── SetPictureFolder   ""   (the pictures folder itself again)
```

**The picture proves the shot.** The Freezer fires the camera through a wire and cannot tell whether its shutter opened: the camera may ignore the release. `ExpectPicture` listens on `/camera/picture` from before the shot, and the objective succeeds only once the camera has reported the picture, within 15 s of the end of the shot. Each shot of [`FocusStack`](FocusStack.md) is checked the same way, so every client that fires a shot, StepIt UI, the editor or the command line, learns the same.

**A test shot goes into the folder `tests`** of the camera's pictures, apart from the folders of the stacks, see [`FocusStack`](FocusStack.md). StepIt UI's **Test shot** runs this objective. After the shot, whether it succeeded or failed, the next pictures go into the pictures folder itself again, not into `tests`.

The shot is the behavior `Shoot`, which another objective uses as one of its steps, e.g. a shot at each position of a stack. The objective cannot have the name of the behavior: BehaviorTree.CPP refuses a tree named like a node.

**The sequences belong to the rig, not to the objective.** Which jack holds a camera, a flash or a light, and the timing of each step, are parameters of the Freezer node, in the section `freezer` of `rig.yaml`. On the rig today, the camera is on OUT8 and the lights on OUT1, and `test_shot` is a `timed_light` sequence: it focuses the camera, opens its shutter, switches the lights on and off, then releases the camera. The exposure of the camera must last until the lights are off, see the timings in `rig.yaml`.

**A shot that has started always runs to its end.** When another objective replaces this one, the commander halts it, but the Freezer refuses to cancel a shot that has started, so that the shutters close: the shot ends on its own, within its duration. A shot ends with every output of the board off, lights included.

It fails when the Freezer node is not running, when the sequence is unknown, or when the board is busy with another shot, e.g. one fired by its remote trigger: the log of the Freezer node says why. It also fails when no picture comes: the camera is off, not connected over USB, not plugged into the jack of the sequence, or ignored the release. A camera set to RAW+JPEG reports two files for a shot: the objective succeeds on the first, and StepIt UI shows both.
