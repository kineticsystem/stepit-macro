# Sourced by the scripts of this folder, inside the stepit-macro container,
# where this repo is mounted at ~/ws.

source /opt/ros/jazzy/setup.bash

# The repo root: rosdep and colcon act on the current directory, so the scripts
# can be called from anywhere.
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../.."

# The rig's plugin is built on top of the workspace that holds the commander,
# with the server and BehaviorTree.ROS2: the commander loads it into its
# process. In the container, the modules' workspace (../modules); CI, which
# builds the commander alone, sets UNDERLAY to the commander's install folder.
source "${UNDERLAY:-install/modules}/setup.bash"

# The packages of src/plugins, built into folders of their own: the
# stepit-macro container builds src/stepit-macro next to them.
BASE_PATHS=(src/plugins)
COLCON_LOG=(--log-base log/plugins)
COLCON_OUTPUT=(--build-base build/plugins --install-base install/plugins)
