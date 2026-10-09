# MarkFar

[`mark_far.xml`](../src/plugins/stepit_objectives/objectives/mark_far.xml) remembers where the rail is as the far end of a focus stack: the camera's position at which the farthest part of the subject that must be sharp is in focus, the closest to the subject. In StepIt UI, it is the **Mark** button below the rail's slider, held for 0.6 s. It works as [`MarkNear`](MarkNear.md), which marks the other end, and saves the position as `far` in the same state file.

It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: MarkFar, payload: ''}"
```

```
MarkFar
├── GetJointPositions   joint2, the rail   -> {rail}
└── SaveValues          far                (state.far of stack_state)
```

[`FocusStack`](FocusStack.md) always shoots from the near mark to the far one, whichever is the larger motor position.
