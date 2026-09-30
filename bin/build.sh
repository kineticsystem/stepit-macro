#!/bin/bash -e

# Build the rig's packages in src, on top of the commander's workspace, which
# provides the server and BehaviorTree.ROS2. Run it inside the commander's
# container, where the rig is mounted at ~/rig, after the commander's build.sh.

source /opt/ros/jazzy/setup.bash
source "${COMMANDER_WS:-$HOME/ws}/install/setup.bash"

cd "$(dirname "$(readlink -f "$0")")/.."

colcon build --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --symlink-install --event-handlers log-
