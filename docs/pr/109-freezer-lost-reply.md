# Update the Modules: the Freezer Tolerates a Lost Reply

Pull request [#109](https://github.com/kineticsystem/stepit-macro/pull/109), branch `freezer-lost-reply`.

## Goal

A focus stack failed at its 46th shot, at the fifth angle: the Freezer node logged `Shot failed: SerialException: timeout`, and `FocusStack` stopped. The shot had fired, and the camera had taken its picture.

## Decisions

**The Freezer asks again.** One status query of the shot got no reply within 0.2 s, and one lost reply failed the stack. StepIt Freezer [#18](https://github.com/kineticsystem/stepit-freezer/pull/18) retries it, up to 3 in a row, and drops a reply that comes late. `modules/stepit-freezer` moves to that branch. Its default, `max_missed_status: 3`, suits the rig: `rig.yaml` does not set it.

**StepIt Motors at its commit on `main`.** [#108](108-inverted-joints.md) pointed `modules/stepit-motors` at the branch of StepIt Motors #26, which was merged by rebase. The pointer moves to the same tree on `main`, `f9df28a`, so that it survives the deletion of the branch.

## Open Items

- Merge StepIt Freezer #18 first, then point `modules/stepit-freezer` at its commit on `main`.
- Why the reply was lost. The lost query came as the board released the camera, 0.2 s after it switched the lights off: noise from the lights is the first suspect. The Freezer node now warns of each shot that missed a reply.
