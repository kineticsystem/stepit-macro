# MoveRailBy

[`move_rail_by.xml`](../src/plugins/stepit_objectives/objectives/move_rail_by.xml) moves the rail, `joint2`, by a distance in millimetres from where it is.

| Parameter | Required | Description |
|---|---|---|
| `mm` | yes | How far to move the rail, in millimetres: positive away from the subject, negative toward it, e.g. `-5.0`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: MoveRailBy, payload: '{mm: -5.0}'}"
```

```
MoveRailBy
├── SubTree EnsureControllers   (position_controller only)
├── GetJointPositions           joint2   -> {start}
├── MillimetresToRadians        {@mm}    -> {offset}, with mm_per_turn.joint2
├── OffsetVector                {start} + {offset}   -> {target}
└── CommandJointPositions       joint2 to {target}
```

**The millimetres come from a measurement.** `MillimetresToRadians` divides by `mm_per_turn.joint2`, in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml): 1.592 mm per turn of the motor, measured on the rig by driving the rail 198 mm and reading the motor's 124.347 turns. Without it, the objective fails and nothing moves.

**Positive is away from the subject**, as the motor counts: the rail's motor turns the positive way when the camera moves away. The rail's slider in StepIt UI is the same way round, up for away.

It moves the rail on the microcontroller's own profile, through the position controller, as [`OffsetJointsDirectlyBy`](OffsetJointsDirectlyBy.md) does, and does not overshoot against backlash: a move toward the subject and one away from it may end a backlash apart.
