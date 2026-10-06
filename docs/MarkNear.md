# MarkNear

[`mark_near.xml`](../src/plugins/stepit_objectives/objectives/mark_near.xml) remembers where the rail is as the near end of a focus stack. We drive the camera with the gamepad or the sliders until the closest part of the subject that must be sharp is in focus, then run it: the camera is then at its farthest from the subject. In StepIt UI, it is the **Mark** button above the rail's slider, on the side the slider moves the camera away. [`MarkFar`](MarkFar.md) marks the other end, and [`FocusStack`](FocusStack.md) shoots from the one to the other.

It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: MarkNear, payload: ''}"
```

```
MarkNear
├── GetJointPositions   joint2, the rail   -> {rail}
└── SaveValues          near               (in the state file)
```

**It moves nothing, and switches no controller.** It only reads the joint states, so the gamepad keeps driving the robot: we can mark one end, drive to the other and mark it, without handing the robot over again.

**The mark is saved in the state file**, the parameter `state_file` in the section `stepit_server` of [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml), `~/ws/state/stack.yaml`: the folder `state` of the repo, which git ignores. It holds one list per name, e.g. `near: [12.4]`, with the far mark if there is one.

**A mark is a count of motor steps** since the controller of the motors powered up. It survives a restart of the rig, but not one of the controller: mark both ends again for every subject.
