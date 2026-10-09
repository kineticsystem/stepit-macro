# MarkNear

[`mark_near.xml`](../src/plugins/stepit_objectives/objectives/mark_near.xml) remembers where the rail is as the near end of a focus stack. We drive the camera with the gamepad or the sliders until the closest part of the subject that must be sharp is in focus, then run it: the camera is then at its farthest from the subject. In StepIt UI, it is the **Mark** button above the rail's slider, on the side the slider moves the camera away, held for 0.6 s. [`MarkFar`](MarkFar.md) marks the other end, and [`FocusStack`](FocusStack.md) shoots from the one to the other.

It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: MarkNear, payload: ''}"
```

```
MarkNear
├── GetJointPositions   joint2, the rail   -> {rail}
└── SaveValues          near               (state.near of stack_state)
```

**It moves nothing, and switches no controller.** It only reads the joint states, so the gamepad keeps driving the robot: we can mark one end, drive to the other and mark it, without handing the robot over again.

**The mark is saved in the state of the rig**, the parameter `state.near` of the node `stack_state`, e.g. `[12.4]`, which every page shows, see [The State of the Rig](../README.md#the-state-of-the-rig). `[]` means not marked.

**A mark is a count of motor steps** since the controller of the motors powered up, which means nothing once it powers up again, as it does when the Pi is switched on. So the marks are kept in memory only, never in a file, and a start of the rig forgets them: `forgotten` in the section `stack_state` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml). Mark both ends again for every subject.
