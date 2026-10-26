# Update the Modules: StepIt Motors on the Teensy 4.0

Pull request [#104](https://github.com/kineticsystem/stepit-macro/pull/104), branch `stepit-motors-teensy-40`.

## Goal

The rig's motors now run on a Teensy 4.0 in place of the Teensy 4.1. StepIt Motors builds its firmware for both boards, `teensy40` by default, and documents both, in [kineticsystem/stepit-motors#25](https://github.com/kineticsystem/stepit-motors/pull/25), whose note, `docs/pr/025-teensy-40-and-41.md`, holds the decisions. Here we move the submodule to its merge, `6f9825f`, and update the Pi guide, which names the board and now flashes it.

## Decisions

**The Pi guide names the new board.** The Teensy 4.0 is `usb-Teensyduino_USB_Serial_21025410-if00`, in the table of the boards and in the `sed` that writes `rig.yaml`. The Pi's own `rig.yaml`, which is not committed, already names it.

**The Pi flashes the Teensy, as it flashes the Freezer.** A new section, Flash StepIt Motors, uses the same PlatformIO, in `~/.platformio/penv`, and PJRC's udev rule, which Raspberry Pi OS Lite needs: without it only root opens the Teensy's HID devices, and the loader cannot write.

**The rig stops while the Teensy is flashed.** The motors' node holds the port. The loader then restarts the Teensy by itself: on the Pi it printed `Unable to soft reboot`, waited, found the bootloader and flashed, with no button pressed. The guide says these messages are harmless.

## Left Out

The default `usb_port` of the motors in `rig.yaml`, `/dev/ttyACM0`. Each computer names its own board by its serial number, as for the Freezer in [#102](102-update-stepit-freezer.md).

## Open Items

- Checked on the Pi, with the Teensy 4.0 and no motors attached: it flashed from the Pi twice, the second time with the rig stopped and the Teensy running StepIt Motors, as the guide says; the driver connected and read firmware 1.1.1; `/joint_states` came at 30 Hz; `OffsetJointsDirectlyBy`, `MoveJointsDirectlyTo` and velocities through `ActivateTeleop` moved the five joints as commanded.
- The pins are not checked with motors on the Teensy 4.0.
- The submodule first pointed at the commit of stepit-motors#25 on its branch, `cbfa061`, and moved to its merge, `6f9825f`, which has the same files.
