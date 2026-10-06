# RotateStageBy

[`rotate_stage_by.xml`](../src/plugins/stepit_objectives/objectives/rotate_stage_by.xml) turns the rotary stage, `joint1`, by an angle in degrees from where it is.

| Parameter | Required | Description |
|---|---|---|
| `deg` | yes | How far to turn the stage, in degrees: negative counter-clockwise, positive clockwise, as the motor counts, e.g. `10.0`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: RotateStageBy, payload: '{deg: 10.0}'}"
```

```
RotateStageBy
├── SubTree EnsureControllers   (position_controller only)
├── GetJointPositions           joint1   -> {start}
├── DegreesToRadians            {@deg}   -> {offset}, with deg_per_turn.joint1
├── OffsetVector                {start} + {offset}   -> {target}
└── CommandJointPositions       joint1 to {target}
```

**The degrees come from the gear of the stage.** `DegreesToRadians` divides by `deg_per_turn.joint1`, in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml): 4.5 degrees per turn of the motor, an 80:1 worm gear. A revolution measured on the rig took 80.098 turns; the 0.098 beyond 80 is the error of lining up a mark by eye, and the backlash. Without it, the objective fails and nothing moves.

**Counter-clockwise is negative**, as the motor counts, seen as when the stage was measured.

It turns the stage on the microcontroller's own profile, through the position controller, as [`OffsetJointsDirectlyBy`](OffsetJointsDirectlyBy.md) does, and does not overshoot against backlash.
