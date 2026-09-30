#!/bin/bash -e

# Install the dependencies of the rig's packages in src. Run it inside the
# commander's container, where the rig is mounted at ~/rig, after the
# commander's own update.sh.

source /opt/ros/jazzy/setup.bash
source "${COMMANDER_WS:-$HOME/ws}/install/setup.bash"

# rosdep and colcon act on the current working directory, so move to the
# workspace root. This lets the script be called from anywhere.
cd "$(dirname "$(readlink -f "$0")")/.."

if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
    sudo rosdep init
fi

rosdep update
rosdep install --ignore-src --from-paths src -y -r
