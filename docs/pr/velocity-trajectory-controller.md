# Update the Modules: the Velocity Trajectory Controller

Branch `velocity-trajectory-controller`.

## Goal

StepIt Motors has a second trajectory controller, `velocity_trajectory_controller`, for a path the rig must follow in time, e.g. a smooth camera move, and its controllers run at 100 Hz instead of 30 Hz, in [kineticsystem/stepit-motors#27](https://github.com/kineticsystem/stepit-motors/pull/27), whose note, `docs/pr/027-velocity-trajectory-controller.md`, holds the decisions and the measurements. Here we move the submodules to `main`.

## Decisions

**All three modules move to `main`.** StepIt Motors brings #27. StepIt Camera moves only to the merge commit of its #18, with no change of code, and StepIt Freezer to its design review, `docs/Review.md`, with no change of code or firmware.

**No objective uses the new controller yet.** It is loaded inactive. `joint_trajectory_controller` stays active, and the objectives keep moving through `position_controller` and `CommandJointPositions`. An objective that follows a path would activate `velocity_trajectory_controller` in `EnsureControllers` and send the trajectory with `FollowJointTrajectory`.

**100 Hz concerns the whole rig.** `/joint_states` comes at 100 Hz, and the motors' serial port carries three times the traffic. StepIt UI does not read `/joint_states`, and `CommandJointPositions` waits on tolerances and a timeout, not on a number of messages, so neither depends on the rate.

## Tested

- On the Pi, a Raspberry Pi 5 with the Teensy 4.0 and the motors: before #27, a cycle of the motors' serial port, one velocity command and one status query, took 1.6 ms on average and 2.7 ms at most over a minute at 100 Hz. On a 7 s ellipse of the stage and the rail, `joint_trajectory_controller` at 30 Hz trailed by up to 1,156 motor steps, the velocity controller at 100 Hz by 3.
- On the Pi, with the submodules at these commits: the controller manager runs at 100 Hz, `/joint_states` comes at 100 Hz, `velocity_trajectory_controller` is loaded inactive, the motors, the Freezer and the camera are connected, and no overrun is logged.
