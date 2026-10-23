# Update the Modules: StepIt Freezer on the Arduino Nano R4

Pull request [#102](https://github.com/kineticsystem/stepit-macro/pull/102), branch `update-stepit-freezer`.

## Goal

The Freezer board of the rig now carries an Arduino Nano R4 in place of the Nano 3. StepIt Freezer ported its firmware, and dropped the Nano 3, in [kineticsystem/stepit-freezer#16](https://github.com/kineticsystem/stepit-freezer/pull/16), whose note, `docs/pr/016-nano-r4.md`, holds the decisions of the port. Here we move the submodule to its merge, `a67e056`, and update the Pi guide, which flashes the board. StepIt Commander moves to `9505417`, the merge of its #25, whose tree is the commit the rig already used: the pointer only lands on its `main`.

## Decisions

**The Pi flashes the board, with PlatformIO.** The firmware is not in the image, and the board is plugged into the Pi. PlatformIO lives in a virtual environment of its own, `~/.platformio/penv`, not in the container: flashing needs the Nano's USB device, and a firmware change is rare.

**A udev rule, as for the cameras.** Raspberry Pi OS Lite leaves the Nano R4's USB device to root, and DFU writes to it. The rule lives in StepIt Freezer, next to the firmware, and the Pi guide installs it.

**The rig finds the Freezer by name.** The Nano R4 is a `cdc_acm` device, `/dev/ttyACM*`, like the Teensy of the motors, so the two ports swap with the order they were plugged in. `rig.yaml` on the Pi names it `/dev/serial/by-id/usb-Arduino_Nano_R4_35062A2239313437DE0133354B573372-if00`; the Pi guide's `sed` writes that name.

**The submodules over HTTPS when the Pi has no GitHub key.** Updating the Pi failed with `Host key verification failed`: `.gitmodules` lists SSH URLs. The repositories are public, so the Pi guide fetches them over HTTPS for that one command, instead of changing `.gitmodules`, which the PCs use with their keys.

## Left Out

The default `usb_port` of the Freezer in `rig.yaml`, `/dev/ttyUSB0`, which no board has any more. It only matters with `use_fake: false`, each computer names its own board by its serial number, and the `sed` of the Pi guide tells the two boards apart by it: the Teensy's is `/dev/ttyACM0`.

## Open Items

- Checked on the Pi: the rig at `25c0daa` built and passed its tests (161 of the plugins, 60 of `src/stepit-macro`, 79 of StepIt UI); the Nano R4 flashed from the Pi; the Freezer's node connected to it; and `test_shot` fired the camera on OUT8 and the lights on OUT1, twice.
- The Nano R4 connected at boot by itself, after a `sudo reboot` and after the Pi was switched off with StepIt UI's `power_off` and on again by hand: each time the kernel found it at once, with no `error -71`, the Freezer's node connected on its first try, and `test_shot` fired. The Nano 3 had to be unplugged and plugged in again after most boots. The Pi guide no longer describes the Nano 3, and its entry for a Freezer missing at boot says so.
- With the submodule at the squashed commit of #16, the Pi followed its own guide: it flashed the Nano R4 from the checkout, the hardware script passed every check, the node connected 57 ms after opening the port, `connect_delay` now 0, and `test_shot` fired.
- The Teensy was not plugged in, so the motors and a full focus stack were not run with the new board.
