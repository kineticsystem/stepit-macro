#!/bin/bash -e

# Test the rig's programs, src/stepit-macro, inside the stepit-macro container.
# The tests use the robot's own topic, action and service names, so on the
# robot's ROS domain they would move it. They always run on a domain of their
# own, 77, or STEPIT_TEST_DOMAIN_ID: whatever ROS_DOMAIN_ID the shell has is the
# robot's, so it is replaced, not kept.
export ROS_DOMAIN_ID="${STEPIT_TEST_DOMAIN_ID:-77}"

source "$(dirname "$(readlink -f "$0")")/common.sh"

colcon "${COLCON_LOG[@]}" test --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" \
    --packages-skip btcpp_ros2_interfaces --return-code-on-test-failure
colcon test-result --test-result-base build/stepit-macro --all --verbose
