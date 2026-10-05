#!/bin/bash -e

# Install the dependencies of the whole rig, inside the stepit-macro container,
# in one rosdep run over the modules and the rig's two workspaces: src/plugins
# depends on packages of the modules, which rosdep finds among the sources it is
# given. bin/plugins/update.sh, which needs the modules built, is for CI, which
# builds the commander first.

bin="$(dirname "$(readlink -f "$0")")"
"$bin/modules/update.sh" src/plugins src/stepit-macro
