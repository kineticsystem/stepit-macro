#!/bin/bash -e

# Install the packages of StepIt UI, ui/, with pnpm, inside the stepit-macro
# container.

cd "$(dirname "$(readlink -f "$0")")/../../ui"
pnpm install --frozen-lockfile --config.confirmModulesPurge=false
