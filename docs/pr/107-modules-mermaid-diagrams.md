# Update the Modules: Diagrams as SVG Images

Pull request [#107](https://github.com/kineticsystem/stepit-macro/pull/107), branch `modules-mermaid-diagrams`.

## Goal

[#103](103-mermaid-diagrams.md) moved the rig's diagrams to Mermaid sources rendered as SVG images. Each module did the same in a pull request of its own. Here the submodules move to the merges of those pull requests:

| Module | From | To |
|---|---|---|
| StepIt Camera | `ff380bd` | `7fa021d`, [kineticsystem/stepit-camera#16](https://github.com/kineticsystem/stepit-camera/pull/16) |
| StepIt Editor | `323acd2` | `45588f4`, [kineticsystem/stepit-editor#23](https://github.com/kineticsystem/stepit-editor/pull/23) |
| StepIt Freezer | `a67e056` | `f4d4db3`, [kineticsystem/stepit-freezer#17](https://github.com/kineticsystem/stepit-freezer/pull/17) |

StepIt Commander and StepIt Motors were already at the head of their `main`.

## Decisions

**No change to the rig.** The modules change only their docs, `CLAUDE.md`, `bin/render-diagrams.sh`, `bin/check-diagrams.sh`, `bin/test.sh` and CI: no code, no launch file, no firmware.
