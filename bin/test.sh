#!/bin/bash -e

# Test the rig's packages in src, inside the commander's container. The tests
# move the real robot if it runs on the same ROS domain: run them on a domain
# of their own, e.g. ROS_DOMAIN_ID=77 ~/rig/bin/test.sh.

source /opt/ros/jazzy/setup.bash
source "${COMMANDER_WS:-$HOME/ws}/install/setup.bash"

cd "$(dirname "$(readlink -f "$0")")/.."

colcon test --return-code-on-test-failure
colcon test-result --all --verbose
