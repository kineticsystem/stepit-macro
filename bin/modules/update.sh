#!/bin/bash -e

# Install the dependencies of the modules, inside the stepit-macro container:
# rosdep's for their ROS packages, and pnpm's for their web pages. Arguments are
# more folders of packages for rosdep, e.g. the rig's: ../update.sh passes them,
# so that rosdep knows the modules' packages they depend on before they are built.

source "$(dirname "$(readlink -f "$0")")/common.sh"

if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
    sudo rosdep init
fi

rosdep update
rosdep install --ignore-src --from-paths "${BASE_PATHS[@]}" "$@" -y -r

for page in "${WEB_PAGES[@]}"; do
    (cd "$page" && pnpm install --frozen-lockfile --config.confirmModulesPurge=false)
done
