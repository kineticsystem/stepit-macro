# The Rig's Plugin

This folder holds the rig's behaviors and objectives. The `stepit-macro` container builds it as a workspace of its own, on top of the modules' workspace, which holds the commander.

| Package | Role |
|---|---|
| `stepit_behaviors` | The behaviors, built as one BehaviorTree.CPP plugin. |
| `stepit_objectives` | The objectives and their subtrees, XML only. The section `stepit_server` of [`rig.yaml`](../stepit-macro/stepit_bringup/config/rig.yaml) tells the commander where to find them. |
| `stepit_tests` | The tests of both, against a fake robot. |

**Why on top of the commander.** The commander loads `stepit_behaviors` into its own process, as a plugin. A plugin must be built against the libraries of the process that loads it, here the commander's BehaviorTree.CPP and BehaviorTree.ROS2: `bin/plugins` sources the modules' workspace first, or the commander's alone in CI, through `UNDERLAY`.

**What the commander reads, and when.** It loads the plugin from `install/plugins` when it starts, so a changed behavior needs a build and a restart of the rig. It reads the XML of the objectives again before each goal, and the StepIt Editor opens [`stepit_objectives/objectives`](stepit_objectives/objectives) directly, so a changed objective needs nothing.

Build and test it in the container. `test.sh` always runs the tests on a ROS domain of their own, 77 or `STEPIT_TEST_DOMAIN_ID`, as they would move the real robot otherwise: never run them with a plain `colcon test`.

```
./docker/dock.sh shell
~/ws/bin/plugins/build.sh
~/ws/bin/plugins/test.sh
```

The rig's own programs, which run on their own rather than inside the commander, e.g. the gamepad, are in [`../stepit-macro`](../stepit-macro), with the launch file of the rig. See [The Workspaces](../../README.md#the-workspaces) and [The Behaviors and Objectives](../../README.md#the-behaviors-and-objectives) in the README.
