#!/bin/bash -e

# Type check and test StepIt UI, ui/, inside the stepit-macro container. The
# tests need no rig: they answer as rosbridge with a fake WebSocket.

cd "$(dirname "$(readlink -f "$0")")/../../ui"
pnpm run typecheck
pnpm run test
