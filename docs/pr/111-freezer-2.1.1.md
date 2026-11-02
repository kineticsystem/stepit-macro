# Update the Modules: StepIt Freezer on Firmware 2.1.1

Pull request [#111](https://github.com/kineticsystem/stepit-macro/pull/111), branch `freezer-2.1.1`.

## Goal

The Nano R4 of the Freezer left about one status reply in 2000 unanswered during a shot, which stopped a focus stack. StepIt Freezer [#19](https://github.com/kineticsystem/stepit-freezer/pull/19) fixes the cause in the firmware, 2.1.1, and [#21](https://github.com/kineticsystem/stepit-freezer/pull/21) removes the recovery of [#109](109-freezer-lost-reply.md), no longer needed.

## Decisions

**`modules/stepit-freezer` at `b5ff1bb`,** on StepIt Freezer's `main`, with both. The Nano of the rig already runs 2.1.1; the node must be built again for the removal of the recovery.
