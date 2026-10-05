#!/bin/bash -e

# Build StepIt UI, ui/, into ui/dist, which the rig serves on port 8070 (see
# rig.launch.py), inside the stepit-macro container. Type checks it first.

cd "$(dirname "$(readlink -f "$0")")/../../ui"
pnpm run build
