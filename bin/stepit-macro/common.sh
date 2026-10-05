# Sourced by the scripts of this folder, inside the stepit-macro container,
# where the repo is mounted at ~/ws.

source /opt/ros/jazzy/setup.bash

# The repo root: rosdep and colcon act on the current directory, so the scripts
# can be called from anywhere.
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../.."

# The packages to build: the rig's programs, and the interfaces of the
# commander, which the gamepad needs to send it goals. BehaviorTree.ROS2 has no
# Debian package, so they are built from the commander's own copy, which
# guarantees they match the commander's. Naming the folder overrides
# modules/COLCON_IGNORE, which only applies to a folder that colcon finds by
# crawling. Built into folders of their own, next to the modules' workspace
# and src/plugins.
BASE_PATHS=(src/stepit-macro modules/stepit-commander/modules/BehaviorTree.ROS2/btcpp_ros2_interfaces)
COLCON_LOG=(--log-base log/stepit-macro)
COLCON_OUTPUT=(--build-base build/stepit-macro --install-base install/stepit-macro)
