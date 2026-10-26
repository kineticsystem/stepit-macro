# Diagrams as SVG Images, from Mermaid Sources

Pull request [#103](https://github.com/kineticsystem/stepit-macro/pull/103), branch `mermaid-diagrams`.

## Goal

Every diagram of the rig's documents in one style, the one of the `beautiful-mermaid-diagrams` skill: colours by role, compact, readable in light and dark mode, as StepIt Speech did first. The modules get the same change in their own pull requests: [stepit-camera#16](https://github.com/kineticsystem/stepit-camera/pull/16), [stepit-editor#23](https://github.com/kineticsystem/stepit-editor/pull/23), [stepit-freezer#17](https://github.com/kineticsystem/stepit-freezer/pull/17) and [stepit-motors#24](https://github.com/kineticsystem/stepit-motors/pull/24). StepIt Commander has no diagram.

## Decisions

**An SVG image, not a Mermaid block.** GitHub stretches a Mermaid block to the width of the page, so a narrow diagram becomes very tall. An image shows at the width we give it. The source stays beside it, in `docs/diagrams/<name>.mmd`, or `ui/docs/diagrams/` for StepIt UI.

**A hash, not a render, in CI.** `bin/render-diagrams.sh` writes the SHA-256 of the source into the first line of the SVG. `bin/check-diagrams.sh` compares the two, so CI needs neither Node.js nor Chrome. Whoever changes a `.mmd` renders it and commits both.

**A shell script, not a test of a workspace.** The diagrams belong to no workspace, and `ci.yml` tests each workspace alone. `ci-format.yml`, which looks at the whole repository, runs the check; `bin/test.sh` runs it last.

**Narrow before faithful to the layout.** The review's system diagram was 1,200 px wide, with every arrow of StepIt UI crossing the others. It is two diagrams now, which tell the rule of the rig: the tasks go through the commander, the configuration straight to the nodes.

## Left Out

The submodule pointers. They move in an update of the modules, once the four pull requests of the modules are merged.

The notes of `docs/pr/` keep their Mermaid blocks: a note is not rewritten after its merge.
