#!/bin/bash -e

# Build the modules, inside the stepit-macro container: their ROS packages, their
# web pages, and the editor's native validator. Arguments are passed to colcon,
# e.g. `--packages-up-to stepit_camera` to build one module's packages only.

source "$(dirname "$(readlink -f "$0")")/common.sh"

check_shared_libraries

colcon "${COLCON_LOG[@]}" build --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" \
    --packages-skip "${SKIP_PACKAGES[@]}" \
    --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=OFF \
    --symlink-install --event-handlers log- "$@"

for page in "${WEB_PAGES[@]}"; do
    (cd "$page" && pnpm run build)
done

# The editor's validator loads the objectives with BehaviorTree.CPP: the ROS
# package, the very library the commander runs them with. Built into the rig's
# build folder, as its own would hold the CMake cache of the editor's container;
# the launch file passes its path to the editor in BTCPP_VALIDATOR.
cmake -S modules/stepit-editor/validator -B build/editor-validator -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/ros/jazzy > /dev/null
cmake --build build/editor-validator --parallel
