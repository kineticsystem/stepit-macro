# Pull Request Notes

Each pull request has a notes file here. It keeps what the pull request does not: what we discussed, what we chose, and why.

## The Notes

| Note | Pull request |
|---|---|
| [102-update-stepit-freezer.md](102-update-stepit-freezer.md) | [#102](https://github.com/kineticsystem/stepit-macro/pull/102) Update the modules: StepIt Freezer on the Arduino Nano R4 |
| [103-mermaid-diagrams.md](103-mermaid-diagrams.md) | [#103](https://github.com/kineticsystem/stepit-macro/pull/103) Diagrams as SVG images, from Mermaid sources |
| [104-stepit-motors-teensy-40.md](104-stepit-motors-teensy-40.md) | [#104](https://github.com/kineticsystem/stepit-macro/pull/104) Update the modules: StepIt Motors on the Teensy 4.0 |
| [105-stepit-motors-on-main.md](105-stepit-motors-on-main.md) | [#105](https://github.com/kineticsystem/stepit-macro/pull/105) StepIt Motors at its commit on `main` |
| [106-test-motors.md](106-test-motors.md) | [#106](https://github.com/kineticsystem/stepit-macro/pull/106) TestMotors: every motor in turn |
| [107-modules-mermaid-diagrams.md](107-modules-mermaid-diagrams.md) | [#107](https://github.com/kineticsystem/stepit-macro/pull/107) Update the modules: diagrams as SVG images |
| [108-inverted-joints.md](108-inverted-joints.md) | [#108](https://github.com/kineticsystem/stepit-macro/pull/108) Invert both motors of the rig |
| [109-freezer-lost-reply.md](109-freezer-lost-reply.md) | [#109](https://github.com/kineticsystem/stepit-macro/pull/109) Update the modules: the Freezer tolerates a lost reply |
| [111-freezer-2.1.1.md](111-freezer-2.1.1.md) | [#111](https://github.com/kineticsystem/stepit-macro/pull/111) Update the modules: StepIt Freezer on firmware 2.1.1 |
| [112-live-view-follows-camera.md](112-live-view-follows-camera.md) | [#112](https://github.com/kineticsystem/stepit-macro/pull/112) The live view button follows the camera |
| [113-pause-live-view-for-a-test-shot.md](113-pause-live-view-for-a-test-shot.md) | [#113](https://github.com/kineticsystem/stepit-macro/pull/113) The live view off for the shots |
| [114-keep-live-view-off-after-a-test-shot.md](114-keep-live-view-off-after-a-test-shot.md) | [#114](https://github.com/kineticsystem/stepit-macro/pull/114) The live view stays off after a test shot |
| [115-rail-not-inverted.md](115-rail-not-inverted.md) | [#115](https://github.com/kineticsystem/stepit-macro/pull/115) The rail not inverted: moves by position go the right way |
| [116-velocity-trajectory-controller.md](116-velocity-trajectory-controller.md) | [#116](https://github.com/kineticsystem/stepit-macro/pull/116) Update the modules: the velocity trajectory controller |
| [117-ros2-workspace.md](117-ros2-workspace.md) | [#117](https://github.com/kineticsystem/stepit-macro/pull/117) The modules from `stepit.repos`, not submodules |
| [118-stepit-motors-on-main-again.md](118-stepit-motors-on-main-again.md) | [#118](https://github.com/kineticsystem/stepit-macro/pull/118) StepIt Motors at its commit on `main`, again |

## How to Write One

- One file per pull request: `<number>-<branch>.md`, e.g. `102-update-stepit-freezer.md`. Before it has a number, use the branch name.
- Sections: **Goal**, **Decisions**, **Left Out**, **Open Items**. Skip the ones that do not apply.
- Short sentences. One idea per paragraph. A diagram when it explains faster.
- Write it while the branch is open. Do not rewrite it after the merge.
- A change to a module is a pull request in its own repository, with its own note there; the note here links it and keeps what concerns the rig.
