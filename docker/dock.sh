#! /bin/bash -e

# Use this script to download, build, start, stop and remove the whole StepIt
# Macro rig: the robot, the commander, the camera, the Freezer board, the editor
# and the gamepad, all in one container, stepit-macro.
#
# The container is defined in docker-compose.yml. This script only adds what
# compose cannot express: the submodules, the X server permission, the host
# uid/gid, and compiling the code of every module inside the container.

# The service that runs the rig.
SERVICE=stepit-macro

# The modules, in ../modules, that the container builds and runs.
MODULES=(stepit-motors stepit-commander stepit-editor stepit-camera stepit-freezer)

function display_usage() {
    echo -e "\nUsage: ./dock.sh <command>\n
    Commands:
    download  Clone the modules, including their own submodules
    build     Build the image, then install the dependencies and compile
              the code of every module and of the rig
    start     Start the rig in the background
    logs      Follow the output of the rig
    status    Show the state of the containers
    shell     Open an interactive terminal into the rig's container
    stop      Stop the rig
    clean     Stop the rig and remove its containers and images\n"
}

function check_modules() {
    local module
    for module in ${MODULES[@]}; do
        if [ ! -f "../modules/$module/README.md" ]; then
            echo "Module '$module' is missing: run ./docker/dock.sh download first." >&2
            exit 1
        fi
    done
}

# Install the dependencies and compile the code of the rig, in a container of
# its image, then save that container as the image: the code lives in the
# bind-mounted repo, so what is built stays on the host, and the dependencies,
# with the marker file of docker-compose.yml, stay in the image, so that the
# rig's container does not install them again.
function compile() {
    local image="$SERVICE:latest"
    local container="$SERVICE-compile"
    echo "Compiling $SERVICE"
    # Refresh the package lists first: the image's own come from a cached
    # layer, and once Ubuntu replaces a package they name a version that is
    # gone, so rosdep's apt-get install fails with 404 Not Found.
    local dependencies="sudo apt-get update && update.sh"
    # The command and labels of the container would replace the image's.
    local cmd=$(docker image inspect --format '{{json .Config.Cmd}}' $image)
    docker rm --force $container &>/dev/null || true
    if ! docker compose run --name $container --no-deps $SERVICE \
        bash -c "$dependencies && touch ~/.dependencies && build.sh"; then
        docker rm $container >/dev/null
        return 1
    fi
    docker commit --change "CMD $cmd" --change "LABEL stepit.compiled=true" \
        $container $image >/dev/null
    docker rm $container >/dev/null
}

# Start the package cache, and wait until it listens, so that the image built
# next downloads through it: set build_args to the proxy argument of the build.
build_args=()
function start_cache() {
    docker compose up --detach --build apt-cache
    local i
    for i in {1..20}; do
        if (exec 3<>/dev/tcp/127.0.0.1/3142) 2>/dev/null; then
            build_args=(--build-arg http_proxy=http://127.0.0.1:3142)
            return 0
        fi
        sleep 0.5
    done
    echo "The package cache is not listening: downloading directly." >&2
}

# Compose resolves the paths in docker-compose.yml against the directory that
# holds it, so every command has to run from there.
cd "$(dirname "$0")"

if [ "$#" -lt 1 ]; then
    echo "Missing required arguments."
    display_usage
    exit 1
fi

command="$1"

# Build args for the Dockerfile: a container user matching the host user.
export USER_UID=$(id -u)
export USER_GID=$(id -g)

# The host's groups of the gamepad (input, which owns /dev/input/js0) and of the
# cameras (plugdev, see udev/60-stepit-camera.rules), which the container's user
# joins: their numbers differ from one system to another, e.g. input is 996 on
# Raspberry Pi OS and 995 on Ubuntu. A group the host lacks falls back to
# dialout, which the user is in anyway.
export INPUT_GID=$(getent group input | cut -d: -f3)
export PLUGDEV_GID=$(getent group plugdev | cut -d: -f3)
export INPUT_GID=${INPUT_GID:-20}
export PLUGDEV_GID=${PLUGDEV_GID:-20}

case "$command" in
    download)
        git -C .. submodule update --init --recursive
        ;;
    build)
        check_modules
        start_cache
        # Rebuilds only the layers that the Dockerfile changed since last time,
        # so there is no need to clean first. The proxy is a predefined build
        # argument: it does not invalidate the cached layers.
        docker compose build "${build_args[@]}" $SERVICE
        compile
        # The images that earlier builds compiled, replaced by this one.
        docker image prune --force --filter label=stepit.compiled=true >/dev/null
        ;;
    start)
        check_modules
        # Allow any local user, including the container, to connect to the X
        # server, for RViz.
        xhost +local: &>/dev/null || true
        # The rig and the package cache, which the first start of a container
        # whose image was not compiled downloads through. --remove-orphans
        # removes the containers of the modules that earlier versions of this
        # file started, one per module, which would hold the same ports.
        docker compose up --detach --remove-orphans
        echo -e "\nStepIt UI is on http://$(hostname -I | awk '{print $1}'):8070"
        echo "The editor is on http://localhost:8080"
        echo "(the ports of src/stepit-macro/stepit_bringup/config/rig.yaml)"
        echo "Follow the output with ./docker/dock.sh logs"
        ;;
    logs)
        docker compose logs --follow $SERVICE
        ;;
    status)
        docker compose ps --all
        ;;
    shell)
        if [ -n "$(docker compose ps --quiet --status running $SERVICE)" ]; then
            docker compose exec $SERVICE bash
        else
            # Not running, e.g. because the rig failed: open a terminal in a
            # fresh container of the same image to find out why.
            echo "$SERVICE is not running: opening a terminal into a new container."
            docker compose run --rm --no-deps $SERVICE bash
        fi
        ;;
    stop)
        docker compose stop $SERVICE
        ;;
    clean)
        docker compose down --rmi all --remove-orphans
        ;;
    *)
        echo "Unknown parameter: $command"
        display_usage
        ;;
esac
