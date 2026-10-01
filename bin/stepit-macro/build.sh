#!/bin/bash -e

# Build the rig's programs, src/stepit-macro, inside the stepit-macro container.

source "$(dirname "$(readlink -f "$0")")/common.sh"

colcon "${COLCON_LOG[@]}" build --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" \
    --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --symlink-install --event-handlers log-
