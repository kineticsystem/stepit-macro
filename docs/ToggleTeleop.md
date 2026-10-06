# ToggleTeleop

[`toggle_teleop.xml`](../src/plugins/stepit_objectives/objectives/toggle_teleop.xml) hands the robot to the gamepad, or takes it back: the gamepad's stop button, button 1, runs it. When `velocity_controller`, the controller the sticks command, is not driving the robot, it runs [`ActivateTeleop`](ActivateTeleop.md). When it is, it activates `joint_trajectory_controller` instead, the controller of the motion objectives, as **Manual drive** in StepIt UI does when switched off.

It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: ToggleTeleop, payload: ''}"
```

```
ToggleTeleop
└── IfThenElse
    ├── IsControllerActive        (velocity_controller? calls /controller_manager/list_controllers)
    ├── SubTree EnsureControllers (yes: joint_trajectory_controller only)
    └── SubTree ActivateTeleop    (no: velocity_controller only)
```

The commander halts the running objective, if any, and runs this one in its place, so a press stops whatever moves the robot, see [Driving the Robot with a Gamepad](Gamepad.md). Either way, the controller that is stopped releases its motors, which brake to 0 at their acceleration, 2 turns/s²; they do not stop dead.

**The objective decides, not the gamepad.** The gamepad does not know which controller is driving the robot: the robot may have been handed over by StepIt UI, or taken back by another objective. `ToggleTeleop` asks the controller manager each time, so the button always does the opposite of the current state. StepIt UI reads the controllers every 2 s, so its **Manual drive** button follows the gamepad.

**`IfThenElse`, not a `Fallback`.** With a `Fallback`, a hand-back that failed, e.g. because the controller manager refused the switch, would go on to `ActivateTeleop` and hand the robot to the gamepad again, reporting success. With `IfThenElse`, the objective fails, and the gamepad's log says `ToggleTeleop failed`.

The controller named in `IsControllerActive` must be the one the gamepad publishes to, `controller` in its [configuration](../src/stepit-macro/stepit_teleop/config/logitech_dual_action.yaml), and the one `ActivateTeleop` activates.
