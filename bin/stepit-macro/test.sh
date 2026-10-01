#!/bin/bash -e

# Test the rig's programs, src/stepit-macro, inside the stepit-macro container.
# The tests publish on the robot's topics: run them on a domain of their own,
# e.g. ROS_DOMAIN_ID=77 test.sh.

source "$(dirname "$(readlink -f "$0")")/common.sh"

colcon "${COLCON_LOG[@]}" test --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" \
    --packages-skip btcpp_ros2_interfaces --return-code-on-test-failure
colcon test-result --test-result-base build/stepit-macro --all --verbose
