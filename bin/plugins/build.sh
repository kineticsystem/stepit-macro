#!/bin/bash -e

# Build the rig's plugin, src/plugins, on top of the modules'
# workspace (../modules/build.sh).

source "$(dirname "$(readlink -f "$0")")/common.sh"

colcon "${COLCON_LOG[@]}" build --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" \
    --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --symlink-install --event-handlers log-
