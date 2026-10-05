#!/bin/bash -e

# Test the rig's two workspaces, inside the stepit-macro container, each on a ROS
# domain of its own choosing (see their test.sh). The modules have their own
# tests, which run in their own CI.

bin="$(dirname "$(readlink -f "$0")")"
"$bin/plugins/test.sh"
"$bin/stepit-macro/test.sh"
