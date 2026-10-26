# TestMotors: Every Motor in Turn

Pull request [#106](https://github.com/kineticsystem/stepit-macro/pull/106), branch `test-motors`.

## Goal

One objective that checks the five motors of the rig: each turns 720 degrees counter-clockwise, then back, one after the other, from `joint1` to `joint5`.

## Decisions

**An objective, not a behavior.** The existing behaviors do every step: `GetJointPositions`, `OffsetVector` and `CommandJointPositions`, through the position controller. So it is XML only, and needs no build.

**A subtree per motor.** `TurnMotorAndBack` takes the joint as its port, and `TestMotors` calls it five times. The commander refuses to run the subtree on its own.

**Back to where it started.** The second move goes to the position read before the first, not by minus two turns, so a motor that stopped short still comes home.

**The angles of the motors, not of the axes.** Two turns are 9 degrees of the rotary stage and about 3 mm of the rail: small enough to run anywhere on the rail. A positive offset is counter-clockwise, as in `OffsetJointsDirectlyBy`.

**Tried on the rig.** It ran on the Raspberry Pi 5, with the real motors, before this pull request.

## Open Items

`docs/OffsetJointsDirectlyBy.md` and `docs/RotateStageBy.md` disagree on which sign is counter-clockwise. The stage's worm gear may turn the stage the other way from its motor; the two documents should say which they mean.
