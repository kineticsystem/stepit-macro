# Inverted Joints

Branch `inverted-joints`.

## Goal

Both motors of the rig turned the other way after their cables were connected again in the aluminium box. The objectives, the gamepad and the sliders of StepIt UI moved the rail and the stage backwards.

## Decisions

**Turned round in software, by StepIt Motors.** StepIt Motors had no way to do it. Its pull request [#26](https://github.com/kineticsystem/stepit-motors/pull/26) adds the launch argument `inverted_joints`: `StepitHardware` negates the positions and the velocities of the joints it names, in both directions. This pull request moves `modules/stepit-motors` to that branch.

**Set in `rig.yaml`.** The section `robot` sets `inverted_joints: joint1 joint2`, like the serial port. A change needs a restart, no build of the rig, and no flash of the Teensy.

**The conventions of the rig hold again.** The rail counts away from the subject and counter-clockwise is negative, as `rig.yaml` says next to `mm_per_turn` and `deg_per_turn`. Nothing else changes: the objectives and the UI keep their signs.

## Left Out

**Swapping the wires of one coil of each motor.** It needs no software, but the box must be opened again, and the next wiring could undo it.

## Open Items

- Merge [stepit-motors#26](https://github.com/kineticsystem/stepit-motors/pull/26) first, then point `modules/stepit-motors` at its commit on `main`.
- On the Raspberry Pi 5: pull, `bin/modules/build.sh --packages-up-to robot_description`, restart, and check that each motor turns the right way, e.g. with `TestMotors`.
