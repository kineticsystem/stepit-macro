# The READMEs Checked Against the Code

Branch `readme-fixes`.

## Goal

Check every statement of the rig's READMEs against the repository as it is, and correct the ones that are wrong: `README.md`, `ui/README.md`, `src/plugins/README.md` and `docs/pr/README.md`.

No code, no launch file and no `rig.yaml` changes: the READMEs say what the repository does today.

## Decisions

The corrections that change what a reader would do:

- **The state of a stack is on `stack_state`, not on the commander.** `ui/README.md` still described the marks as saved in the state file, shown as the commander's parameters, with defaults in a section `focus_stack` of `rig.yaml`. The marks are in memory only, the parameters `state.*` belong to the node `stack_state`, and the defaults are its `defaults`.
- **The page follows one topic to know what runs**, `/stepit_server/objective`. It does not read the status topic of the commander's action.
- **The Freezer board is an Arduino Nano R4, on a `/dev/ttyACM*` port.** The example names it by id as `usb-Arduino_Nano_R4_...-if00`, not as an FTDI adapter. The table keeps `/dev/ttyUSB0`, the value of `rig.yaml`, and says that it is a leftover.
- **The modules are fetched over HTTPS.** Only the clone of this repository, and a push, need an SSH key.
- **The camera's test page is not turned off, it is not built.** `rig.yaml` turns off the camera's rosbridge only; its web server runs on port 8090, for the pictures. The Freezer's page and rosbridge are off.
- **The test pages of the modules do not run with `pnpm dev` in the rig.** The rig installs the packages of the editor's page only, and the Freezer's page expects its own rosbridge on port 9092, which is off. The example is removed.
- **The objective test named as an example exists**: `test_direct_objectives`, not `test_offset_joints_by_objective`.

The rest fills lists that claimed to be complete: the packages `stepit_teleop_msgs` and `stepit_macro_msgs`, the behavior `IsControllerActive`, the folders `devices` and `stack` of StepIt UI and `devices.test.ts`, the device icons of the top bar, the launch arguments `inverted_joints` and `stream`, and the check of the diagrams in `test` and in CI. The spelling is British throughout.

## Left Out

- The modules' own READMEs: they have their own pull requests.
- `.claude/CLAUDE.md`, which repeats that the camera's test page is turned off in `rig.yaml`.
- The comments of the code that repeat the same errors, e.g. `focus_stack` in `ui/src/stack/store.ts` and the test pages in `bin/modules/common.sh`.

## Open Items

- `rig.yaml` sets the Freezer's `usb_port` to `/dev/ttyUSB0`: the Nano R4 is `/dev/ttyACM*`. It matters once `use_fake` is false. `docs/pi5.md` replaces that value with `sed`, so both change together.
- `README.md` says that the rail sits on the rotary stage; `ui/README.md` and `rig.yaml` say that the subject turns on it. Not checked against the rig.
- `README.md` says that an offset of -6.28 turns `joint1` and `joint3` one turn clockwise, while `rig.yaml` inverts `joint1` alone. Not checked against the rig.
