# Pull Request Notes

Each pull request has a notes file here. It keeps what the pull request does not: what we discussed, what we chose, and why.

## The Notes

| Note | Pull request |
|---|---|
| [102-update-stepit-freezer.md](102-update-stepit-freezer.md) | [#102](https://github.com/kineticsystem/stepit-macro/pull/102) Update the modules: StepIt Freezer on the Arduino Nano R4 |
| [103-mermaid-diagrams.md](103-mermaid-diagrams.md) | [#103](https://github.com/kineticsystem/stepit-macro/pull/103) Diagrams as SVG images, from Mermaid sources |
| [104-stepit-motors-teensy-40.md](104-stepit-motors-teensy-40.md) | [#104](https://github.com/kineticsystem/stepit-macro/pull/104) Update the modules: StepIt Motors on the Teensy 4.0 |

## How to Write One

- One file per pull request: `<number>-<branch>.md`, e.g. `102-update-stepit-freezer.md`. Before it has a number, use the branch name.
- Sections: **Goal**, **Decisions**, **Left Out**, **Open Items**. Skip the ones that do not apply.
- Short sentences. One idea per paragraph. A diagram when it explains faster.
- Write it while the branch is open. Do not rewrite it after the merge.
- A change to a module is a pull request in its own repository, with its own note there; the note here links it and keeps what concerns the rig.
