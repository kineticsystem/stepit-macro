# The Live View Button Follows the Camera

Pull request [#112](https://github.com/kineticsystem/stepit-macro/pull/112), branch `live-view-follows-camera`.

## Goal

The Live view button of StepIt UI showed what its own page had asked for, never what the camera did. Switched off from elsewhere, e.g. another page, the button stayed on. A page that had not started the live view also did not stop it before a shot or a stack.

## Decisions

**The camera says it.** StepIt Camera [#17](https://github.com/kineticsystem/stepit-camera/pull/17) publishes `streaming` on `/camera/status`, as soon as the live view is switched. `modules/stepit-camera` moves to that branch.

**The page follows it.** `followCamera` sets `streaming` from the status: every page shows the same, whoever switched it, as the rest of the rig. A click still shows its choice at once, and the status confirms it.

**No restart after a reconnection.** The page started the live view again after a reconnection if it had been on. The camera now owns the state: after a restart of the rig, the live view is what the camera says.

## Open Items

- Merge StepIt Camera #17 first, then point `modules/stepit-camera` at its commit on `main`.
