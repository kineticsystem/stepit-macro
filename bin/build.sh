#!/bin/bash -e

# Build the whole rig, inside the stepit-macro container: the modules, then the
# rig's plugin on top of them, then the rig's own programs.

bin="$(dirname "$(readlink -f "$0")")"
cd "$bin/.."

# A workspace built at another path, e.g. src/plugins by the commander's
# container of older versions, which mounted this repo at ~/rig, has CMake
# caches that CMake refuses here, and links into that path: build it again from
# scratch. Its folders hold nothing but what the build makes.
for workspace in modules plugins stepit-macro; do
    for cache in build/$workspace/*/CMakeCache.txt; do
        if [ -f "$cache" ] && ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=$PWD/" "$cache"; then
            echo "build/$workspace was built at another path: removing it and install/$workspace."
            rm -rf build/$workspace install/$workspace
            break
        fi
    done
done

"$bin/modules/build.sh"
"$bin/plugins/build.sh"
"$bin/stepit-macro/build.sh"
