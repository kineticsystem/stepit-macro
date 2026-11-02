# Show the Missed Replies

Pull request [#110](https://github.com/kineticsystem/stepit-macro/pull/110), branch `show-missed-replies`.

## Goal

StepIt Freezer now asks again when its board leaves a status query unanswered, so one lost reply no longer stops a focus stack. A healthy board misses none, though: a miss is still a fault, and it must be seen, not only read in the log of the Freezer node.

## Decisions

**The Freezer at its fix and its report.** `modules/stepit-freezer` moves to the branch of StepIt Freezer's report of the missed replies ([#20](https://github.com/kineticsystem/stepit-freezer/pull/20)), which sits on top of firmware 2.1.1, the fix of the misses ([#19](https://github.com/kineticsystem/stepit-freezer/pull/19)). The Nano of the rig already runs 2.1.1.

**The Freezer's icon turns orange.** `/freezer/status` carries `missed_status`, the replies the board missed during the shots since the node started. StepIt UI shows the icon of the Freezer orange when it is more than 0, a new device state, `warning`, and a tap tells how many and what to check: the firmware and the USB cable.

**The count lives on the node.** Every page shows the same, as the rest of the rig: a page opened late sees the misses of the shots before it. A restart of the rig clears it.

**The other devices are untouched.** Only the Freezer's status has the field; the type of a status message says it is the Freezer's only.

## Open Items

- Merge StepIt Freezer #19, then #20, then point `modules/stepit-freezer` at their commit on `main`.
