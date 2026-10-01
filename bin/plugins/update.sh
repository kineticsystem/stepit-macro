#!/bin/bash -e

# Install the dependencies of the rig's plugin, src/plugins, inside the
# stepit-commander container, after the commander's own update.sh.

source "$(dirname "$(readlink -f "$0")")/common.sh"

if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
    sudo rosdep init
fi

rosdep update
rosdep install --ignore-src --from-paths "${BASE_PATHS[@]}" -y -r
