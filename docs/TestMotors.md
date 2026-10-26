# TestMotors

[`test_motors.xml`](../src/plugins/stepit_objectives/objectives/test_motors.xml) tests the five motors one after the other: each turns 720 degrees, two turns, counter-clockwise, then back to where it was, before the next one starts, from `joint1` to `joint5`. It takes no parameters.

```bash
ros2 action send_goal /commander/execute_objective \
  btcpp_ros2_interfaces/action/ExecuteTree \
  "{target_tree: TestMotors}"
```

```
TestMotors
├── SubTree EnsureControllers   (position_controller only)
└── SubTree TurnMotorAndBack    joint1, then joint2, joint3, joint4, joint5
    ├── GetJointPositions       {joint}   -> {start}
    ├── OffsetVector            {start} + 4π   -> {target}
    ├── CommandJointPositions   {joint} to {target}
    └── CommandJointPositions   {joint} back to {start}
```

**The angles are those of the motors**, not of the axes: two turns of `joint1` turn the rotary stage by 9 degrees, and two turns of `joint2` move the rail by about 3 mm. A positive offset turns a StepIt motor counter-clockwise, as in [`OffsetJointsDirectlyBy`](OffsetJointsDirectlyBy.md).

**Each motor goes back to where it started**, not by minus two turns: the second move is sent to the position read before the first.

Every move goes through the position controller, on the microcontroller's own profile. The objective fails as soon as a motor is not at its target within 60 s, and leaves the next ones untested. Cancelling it deactivates the position controller, and the hardware brakes the motor to rest.
