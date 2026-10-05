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
└── Shoot   (sends the goal to /freezer/shoot, and waits for the end of the shot)
```

The objective is the behavior `Shoot` alone, which another objective uses as one of its steps, e.g. a shot at each position of a stack. The objective cannot have the name of the behavior: BehaviorTree.CPP refuses a tree named like a node.

**The sequences belong to the rig, not to the objective.** Which jack holds a camera, a flash or a light, and the timing of each step, are parameters of the Freezer node, in the section `freezer` of `rig.yaml`. On the rig today, the camera is on OUT8 and the lights on OUT1, and `test_shot` is a `timed_light` sequence: it focuses the camera, opens its shutter, switches the lights on and off, then releases the camera. The exposure of the camera must last until the lights are off, see the timings in `rig.yaml`.

**A shot that has started always runs to its end.** When another objective replaces this one, the commander halts it, but the Freezer refuses to cancel a shot that has started, so that the shutters close: the shot ends on its own, within its duration. A shot ends with every output of the board off, lights included.

It fails when the Freezer node is not running, when the sequence is unknown, or when the board is busy with another shot, e.g. one fired by its remote trigger. The log of the Freezer node says why.
