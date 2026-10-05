#!/bin/bash -e

# Build the whole rig, inside the stepit-macro container: the modules, then the
# rig's plugin on top of them, then the rig's own programs, and StepIt UI.

bin="$(dirname "$(readlink -f "$0")")"
cd "$bin/.."

# A workspace built from sources that are no longer where its CMake caches say,
# e.g. src/plugins built by the commander's container of older versions, which
# mounted this repo at ~/rig, or a module whose folder was renamed, has caches
# that CMake refuses, and links into the old folders: build it again from
# scratch. Its folders hold nothing but what the build makes.
for workspace in modules plugins stepit-macro; do
    for cache in build/$workspace/*/CMakeCache.txt; do
        [ -f "$cache" ] || continue
        sources=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$cache")
        if [[ "$sources" != "$PWD/"* ]] || [ ! -d "$sources" ]; then
            echo "build/$workspace was built from $sources: removing it and install/$workspace."
            rm -rf build/$workspace install/$workspace
            break
        fi
    done
done

"$bin/modules/build.sh"
"$bin/plugins/build.sh"
"$bin/stepit-macro/build.sh"
"$bin/ui/build.sh"
