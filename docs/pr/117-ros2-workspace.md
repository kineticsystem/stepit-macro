# The Modules from `stepit.repos`, Not Submodules

Pull request [#117](https://github.com/kineticsystem/stepit-macro/pull/117), branch `ros2-workspace`.

## Goal

Take the modules, and the libraries they share, from one `.repos` file instead of git submodules, the way a ROS 2 workspace usually brings packages together.

StepIt Motors and StepIt Freezer both carried `serial` and `framed-serial` as submodules of their own, and the commander carried BehaviorTree.ROS2. The rig found each library twice, built the driver's copy, and checked that the Freezer's was at the same commit.

## Decisions

- **One list, `stepit.repos`.** It pins the five modules and the three shared libraries, each once, to commits. `./docker/dock.sh download` imports them with `vcs` into `modules/`, which git ignores, in a container of `ros:jazzy-ros-base`, so the host needs only Docker. The URLs are HTTPS, so the Pi and CI need no key.
- **The modules move in their own pull requests:** [stepit-motors#28](https://github.com/kineticsystem/stepit-motors/pull/28), [stepit-freezer#23](https://github.com/kineticsystem/stepit-freezer/pull/23), [stepit-commander#26](https://github.com/kineticsystem/stepit-commander/pull/26), and the README of [framed-serial#1](https://github.com/kineticsystem/framed-serial/pull/1). Each pins its libraries in a `.repos` file of its own, for building alone.
- **The rig builds its own copies.** `BASE_PATHS` names `modules/serial`, `modules/framed-serial` and `modules/BehaviorTree.ROS2`, never a module's own `modules/`, which a module's container fills when it runs on this checkout. colcon refuses two packages of the same name.
- **`check_shared_libraries` compares files, not checkouts.** It fails when a module's own `.repos` file pins a library at another commit than `stepit.repos`: the module would run against a combination its CI never built. CI runs it too.
- **`download` never deletes.** An old checkout keeps its submodules in `modules/` after the pull, and `vcs` cannot import over them. `download` finds them, stops, and prints the command that removes them.
- **colcon names the folders, as before.** `modules/COLCON_IGNORE` stays, so a plain `colcon build` at the root never builds the modules.

## Left Out

- **Releasing the libraries as Debian packages with bloom.** More work than a fork of `serial` and a library used only by our projects need.

## Open Items

- The modules merge first. StepIt Motors rebases on merge, which gives its commits new hashes: `stepit.repos` then moves to the commits of the three modules on `main`, before this merges.
- Each checkout of the rig, the PC and the Pi, runs the cleanup that `download` prints once, after the pull.
