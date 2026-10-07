# MoveRailToMark

[`move_rail_to_mark.xml`](../src/plugins/stepit_objectives/objectives/move_rail_to_mark.xml) moves the rail back to a mark of the stack, to check the focus there, then hands the robot back to manual drive, so that we can adjust the camera and mark again. In StepIt UI, it is a tap on a **Mark** button, once both ends are marked.

| Parameter | Required | Description |
|---|---|---|
| `mark` | yes | The mark to go to, as [`MarkNear`](MarkNear.md) and [`MarkFar`](MarkFar.md) saved it: `near` or `far`. |

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: MoveRailToMark, payload: '{mark: near}'}"
```

```
MoveRailToMark
├── LoadValues              {@mark}, near, far   (the marks, from the state file)
├── SubTree EnsureControllers   (position_controller only)
├── CommandJointPositions   joint2 to the mark, approaching it from near toward far
└── SubTree ActivateTeleop      (manual drive again)
```

**The rail stands where the stack will put it.** It approaches the mark going from the near mark toward the far one, as every move of [`FocusStack`](FocusStack.md) does: a move the other way first goes past the mark by the rail's overshoot, `overshoot.joint2` of `rig.yaml`, then comes back to it. A move that ends the way a stack would end may otherwise stand a backlash away from where the stack's picture will be taken. The other joints stay where they are.

It needs both marks, for the direction of the approach: with one missing, it fails before anything moves, and StepIt UI says to mark both ends instead of running it.
