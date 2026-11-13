# StepIt Motors at Its Commit on `main`, Again

Branch `stepit-motors-on-main-again`.

## Goal

Move StepIt Motors in `stepit.repos` to its commit on `main`, `dc269f8`.

Pull request #117 merged before its pin moved there. It pins `6ba4a9b`, the tip of [stepit-motors#28](https://github.com/kineticsystem/stepit-motors/pull/28), which StepIt Motors rebased on merge into `dc269f8`, the same tree. Its branch is deleted, so `6ba4a9b` is only under `refs/pull/28/head`, which a clone never fetches: `download` and CI fail on it.

## Decisions

- **The same tree, another commit.** `git diff 6ba4a9b dc269f8` is empty: nothing changes in the rig.

## Open Items

- A pull request of StepIt Macro that moves a module merges after the module's, and moves to the module's commit on `main` first, as #105 did.
