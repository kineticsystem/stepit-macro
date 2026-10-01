# Sourced by the scripts of this folder, inside the stepit-commander container,
# where the rig is mounted at ~/rig and the commander's own workspace at ~/ws.

source /opt/ros/jazzy/setup.bash
# The rig's plugin is built on top of the commander's workspace, which provides
# the server and BehaviorTree.ROS2: the commander loads it into its process.
source "${COMMANDER_WS:-$HOME/ws}/install/setup.bash"

# The repo root: rosdep and colcon act on the current directory, so the scripts
# can be called from anywhere.
cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/../.."

# The packages of src/plugins, built into folders of their own: the
# stepit-macro container builds src/stepit-macro next to them.
BASE_PATHS=(src/plugins)
COLCON_LOG=(--log-base log/plugins)
COLCON_OUTPUT=(--build-base build/plugins --install-base install/plugins)
