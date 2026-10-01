# ActivateTeleop

[`activate_teleop.xml`](../src/plugins/stepit_objectives/objectives/activate_teleop.xml) hands the robot to the gamepad: it stops whichever controllers are driving the robot and activates `velocity_controller`, which the gamepad's sticks command.

It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: ActivateTeleop, payload: ''}"
```

```
ActivateTeleop
└── SubTree EnsureControllers   (velocity_controller only)
    ├── GetActiveControllers    (calls /controller_manager/list_controllers)
    └── SwitchController        (calls /controller_manager/switch_controller)
```

The gamepad's stop button runs it; the commander halts the running objective, if any, and runs this one in its place, see [Driving the Robot with a Gamepad](Gamepad.md). The motors released by the other controllers brake to 0 at their acceleration, 2 turns/s²; they do not stop dead.

**What handing the robot to the user means lives here, not in the gamepad's code.** The gamepad only asks for the objective named by its parameter `objective`, so a change to this XML takes effect on the next press, with no build. Another objective hands the robot to the user the same way, as a step of its own:

```xml
<SubTree ID="ActivateTeleop"/>
```

It works like [`ActivateController`](ActivateController.md) with a fixed controller: broadcasters keep running, and running it while `velocity_controller` is already active changes nothing. The controller named here must be the one the gamepad publishes to, `controller` in its [configuration](../src/stepit-macro/stepit_teleop/config/logitech_dual_action.yaml).
