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
# The libraries the modules share are built from the rig's own copies, which
# stepit.repos lists once: serial, framed-serial and BehaviorTree.ROS2. A
# module's own modules/ folder, where its own update.sh imports them when its
# own container runs on this checkout, is never built here: colcon refuses two
# packages of the same name.
BASE_PATHS=(
    modules/stepit-motors/src
    modules/stepit-freezer/src
    modules/stepit-camera/src
    modules/stepit-commander/src
    modules/serial
    modules/framed-serial
    modules/BehaviorTree.ROS2
)
# The tests of the modules run in their own CI, and the examples are not used.
SKIP_PACKAGES=(stepit_hardware_tests stepit_camera_tests stepit_server_tests btcpp_ros2_samples)
COLCON_LOG=(--log-base log/modules)
COLCON_OUTPUT=(--build-base build/modules --install-base install/modules)

# The web pages, which each module builds with pnpm into its own dist folder:
# the editor. The test pages of the camera and of the Freezer are only for their
# modules' own containers: rig.yaml turns them off. StepIt UI, the rig's own
# page, is in ui/, with scripts of its own in bin/ui.
WEB_PAGES=(modules/stepit-editor)

# Fail if a module's own .repos file pins a library at another commit than
# stepit.repos: the module would run against the rig's copy, a combination its
# own CI never built. Each module of stepit.repos is checked, so a new module
# with a .repos file of its own is checked too.
function check_shared_libraries() {
    python3 - <<'EOF'
import glob
import sys

import yaml


def pins(path):
    with open(path) as file:
        repositories = yaml.safe_load(file)["repositories"]
    return {name: str(repository["version"]) for name, repository in repositories.items()}


rig = pins("stepit.repos")
different = False
for module in rig:
    for path in glob.glob(f"modules/{module}/*.repos"):
        for name, version in pins(path).items():
            if rig.get(name) != version:
                print(f"{path} pins {name} at {version}, stepit.repos at {rig.get(name)}.", file=sys.stderr)
                different = True
if different:
    print("Move the rig and the modules to the same commits.", file=sys.stderr)
    sys.exit(1)
EOF
}
