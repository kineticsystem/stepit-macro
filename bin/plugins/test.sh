#!/bin/bash -e

# Test the rig's plugin, src/plugins, inside the stepit-commander container.
# The tests move the real robot if it runs on the same ROS domain: run them on
# a domain of their own, e.g. ROS_DOMAIN_ID=77 ~/rig/bin/plugins/test.sh.

source "$(dirname "$(readlink -f "$0")")/common.sh"

colcon "${COLCON_LOG[@]}" test --base-paths "${BASE_PATHS[@]}" "${COLCON_OUTPUT[@]}" --return-code-on-test-failure
colcon test-result --test-result-base build/plugins --all --verbose
