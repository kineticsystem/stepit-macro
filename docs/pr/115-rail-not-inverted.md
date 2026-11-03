# The Rail Not Inverted

Pull request [#115](https://github.com/kineticsystem/stepit-macro/pull/115), branch `rail-not-inverted`.

## Goal

Asked by voice to move the camera 20 millimetres farther, the rig moved it closer. `MoveRailBy`, the focus stacks and the marks moved the rail the wrong way; the sliders of StepIt UI and the gamepad moved it the right way.

## Decisions

**Only the stage is inverted.** [#108](108-inverted-joints.md) inverted both motors, `inverted_joints: joint1 joint2`. The rail's motor did not need it: inverted, its position counted toward the subject, and every objective that moves it by a position went the wrong way. `inverted_joints` is now `joint1`.

**The rail's sliders and stick are positive again.** Since [`ebfd378`](https://github.com/kineticsystem/stepit-macro/commit/ebfd378), `ui_teleop` and `logitech_dual_action.yaml` gave the rail a negative scale, which made up for the inversion: two sign changes, so manual driving looked right. Without the inversion, both scales are positive, and the sliders and the stick move the rail as before.

**Checked on the rig, by voice.** On the Raspberry Pi 5, with this `rig.yaml`: "move the camera 20 millimetres farther" moved it away from the subject, "closer" toward it, and the sliders of StepIt UI kept their direction.

## Open Items

- On the Raspberry Pi 5: its `rig.yaml` already has the fix, by hand; the gamepad's file needs this branch and a restart. Until then, the gamepad's right stick moves the rail backwards.
