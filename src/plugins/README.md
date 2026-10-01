# The Rig's Plugin

This folder holds the rig's behaviors and objectives. It is not built or run in a container of StepIt Macro: the `stepit-commander` container mounts this repo at `~/rig`, and compiles this folder there, on top of the commander's own workspace.

| Package | Role |
|---|---|
| `stepit_behaviors` | The behaviors, built as one BehaviorTree.CPP plugin. |
| `stepit_objectives` | The objectives and their subtrees, XML only, and `config/commander.yaml`, which tells the commander where to find them. |
| `stepit_tests` | The tests of both, against a fake robot. |

**Why in the commander's container.** The commander loads `stepit_behaviors` into its own process, as a plugin. A plugin must be built against the libraries of the process that loads it, here the commander's BehaviorTree.CPP and BehaviorTree.ROS2, which only its container has.

**What the commander reads, and when.** It loads the plugin from `install/plugins` when it starts, so a changed behavior needs a build and a restart of the commander. It reads the XML of the objectives again before each goal, and the StepIt Editor opens [`stepit_objectives/objectives`](stepit_objectives/objectives) directly, so a changed objective needs nothing.

Build and test it in the commander's container, the tests on a ROS domain of their own, as they would move the real robot otherwise:

```
./docker/dock.sh build stepit-commander
./docker/dock.sh shell stepit-commander
~/rig/bin/plugins/build.sh
ROS_DOMAIN_ID=77 ~/rig/bin/plugins/test.sh
```

The rig's own programs, which run on their own rather than inside the commander, e.g. the gamepad, are in [`../stepit-macro`](../stepit-macro), built in the `stepit-macro` container. See [The Two Workspaces](../../README.md#the-two-workspaces) and [The Behaviors and Objectives](../../README.md#the-behaviors-and-objectives) in the README.
