# Sourced by the scripts of this folder, inside the stepit-macro container,
# where this repo is mounted at ~/ws.
#
# The ROS packages of the modules, built as one workspace, underneath the rig's
# own two: src/plugins on top of it, and src/stepit-macro next to them. Each
# module keeps its own container and scripts, which build it alone into its own
# build folders: these build it into the rig's, so the two never share a CMake
# cache, whose paths differ.

source /opt/ros/jazzy/setup.bash

# The repo root: rosdep and colcon act on the current directory, so the scripts
# can be called from anywhere.
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../.."

# The folders of the packages, named one by one: colcon never crawls modules/.
# The driver and the Freezer both carry serial and framed-serial as submodules
# of their own: colcon refuses two packages of the same name, so the driver's
# are built, and check_shared_libraries makes sure the Freezer's are the same.
BASE_PATHS=(
    modules/stepit-motors/src
    modules/stepit-motors/modules
    modules/stepit-freezer/src
    modules/stepit-camera/src
    modules/stepit-commander/src
    modules/stepit-commander/modules/BehaviorTree.ROS2
)
# The tests of the modules run in their own CI, and the examples are not used.
SKIP_PACKAGES=(stepit_hardware_tests stepit_camera_tests stepit_server_tests btcpp_ros2_samples)
COLCON_LOG=(--log-base log/modules)
COLCON_OUTPUT=(--build-base build/modules --install-base install/modules)

# The web pages, which each module builds with pnpm into its own dist folder.
WEB_PAGES=(modules/stepit-camera/web modules/stepit-freezer/web modules/stepit-editor)

# Fail if the driver and the Freezer pin different commits of a library they
# share: the Freezer would run against the driver's copy without telling.
function check_shared_libraries() {
    local library driver freezer
    for library in serial framed-serial; do
        driver=$(git -C modules/stepit-motors/modules/$library rev-parse HEAD)
        freezer=$(git -C modules/stepit-freezer/modules/$library rev-parse HEAD)
        if [ "$driver" != "$freezer" ]; then
            echo "stepit-motors and stepit-freezer pin different commits of $library:" >&2
            echo "  $driver and $freezer. Move both modules to the same one." >&2
            exit 1
        fi
    done
}
