# StepIt Motors at Its Commit on `main`

Pull request [#105](https://github.com/kineticsystem/stepit-macro/pull/105), branch `stepit-motors-on-main`.

## Goal

[#104](104-stepit-motors-teensy-40.md) moved the submodule to `cbfa061`, the commit of [kineticsystem/stepit-motors#25](https://github.com/kineticsystem/stepit-motors/pull/25) on its branch. #25 was merged as a new commit, `6f9825f`, and its branch deleted, so `cbfa061` is on no branch of StepIt Motors: only GitHub's ref of the pull request, `refs/pull/25/head`, keeps it. The commit that moved the submodule to `6f9825f` was pushed seconds after #104 was merged, and missed it. Here the submodule moves to `6f9825f`.

## Decisions

**A pull request of its own.** The note of #104 is not rewritten after its merge.

**No change to the rig.** `cbfa061` and `6f9825f` have the same files: the Teensy 4.0 on the rig's Pi runs the same firmware.
