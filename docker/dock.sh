#! /bin/bash -e

# Use this script to download, build, start, stop and remove the whole StepIt
# Macro rig: the robot, the commander, the rig's own programs, the editor and
# the camera, each in its own container.
#
# The containers are defined in docker-compose.yml, which extends the compose
# file of each module. This script only adds what compose cannot express: the
# submodules, the X server permission, the host uid/gid, and compiling the code
# of each module inside its container.

# The services of docker-compose.yml, in the order they are built and started.
SERVICES=(stepit-driver stepit-commander stepit-macro stepit-editor stepit-camera)

# The services that run a module, from its own folder in ../modules. The others
# are this repo's own, e.g. stepit-macro.
MODULES=(stepit-driver stepit-commander stepit-editor stepit-camera)

function display_usage() {
    echo -e "\nUsage: ./dock.sh <command> [service]\n
    Commands:
    download  Clone the modules, including their own submodules
    build     Build the images, then install the dependencies and compile
              the code of every module, or of the given service only
    start     Start everything in the background
    logs      Follow the output of every service, or of the given one
    status    Show the state of every container
    shell     Open an interactive terminal into a service
              Usage: ./dock.sh shell <service>
    stop      Stop everything
    clean     Stop everything and remove the containers and images

    Services: ${SERVICES[*]}\n"
}

function check_service() {
    local service="$1"
    if [[ ! " ${SERVICES[*]} " =~ " ${service} " ]]; then
        echo "Unknown service: $service" >&2
        display_usage
        exit 1
    fi
}

function check_modules() {
    local module
    for module in ${MODULES[@]}; do
        if [ ! -f "../modules/$module/docker/docker-compose.yml" ]; then
            echo "Module '$module' is missing: run ./docker/dock.sh download first." >&2
            exit 1
        fi
    done
}

# Install the dependencies and compile the code of a service, in a container
# of its image, then save that container as the service's image: the code lives
# in the bind-mounted module, so what is built stays on the host, and the
# dependencies, with the marker file of docker-compose.yml, stay in the image,
# so that the service's own container does not install them again.
function compile() {
    local service="$1"
    local image="$service:latest"
    local container="$service-compile"
    echo "Compiling $service"
    # Refresh the package lists first: the image's own come from a cached
    # layer, and once Ubuntu replaces a package they name a version that is
    # gone, so rosdep's apt-get install fails with 404 Not Found.
    local dependencies="sudo apt-get update && update.sh"
    local builds="build.sh"
    # The commander also builds the rig's plugin, its behaviors and objectives
    # in ../src/plugins, on top of its workspace.
    if [ "$service" = stepit-commander ]; then
        dependencies="$dependencies && ~/rig/bin/plugins/update.sh"
        builds="$builds && ~/rig/bin/plugins/build.sh"
    fi
    # The command and labels of the container would replace the image's.
    local cmd=$(docker image inspect --format '{{json .Config.Cmd}}' $image)
    docker rm --force $container &>/dev/null || true
    if ! docker compose run --name $container --no-deps $service \
        bash -c "$dependencies && touch ~/.dependencies && $builds"; then
        docker rm $container >/dev/null
        return 1
    fi
    docker commit --change "CMD $cmd" --change "LABEL stepit.compiled=true" \
        $container $image >/dev/null
    docker rm $container >/dev/null
}

# Start the package cache, and wait until it listens, so that the images built
# next download through it: set build_args to the proxy argument of the builds.
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
service="$2"
if [ -n "$service" ]; then
    check_service $service
fi

# Build args for the Dockerfiles: a container user matching the host user.
export USER_UID=$(id -u)
export USER_GID=$(id -g)

case "$command" in
    download)
        git -C .. submodule update --init --recursive
        ;;
    build)
        check_modules
        start_cache
        # Rebuilds only the layers that a Dockerfile changed since last time,
        # so there is no need to clean first. The proxy is a predefined build
        # argument: it does not invalidate the cached layers.
        docker compose build "${build_args[@]}" $service
        for s in ${service:-${SERVICES[@]}}; do
            compile $s
        done
        # The images that earlier builds compiled, replaced by these.
        docker image prune --force --filter label=stepit.compiled=true >/dev/null
        ;;
    start)
        check_modules
        # Allow any local user, including the containers, to connect to the X
        # server, for RViz.
        xhost +local: &>/dev/null || true
        docker compose up --detach $service
        echo -e "\nThe editor is on http://localhost:${EDITOR_PORT:-8080}"
        echo "The camera's test page is on http://localhost:8090"
        echo "Follow the output with ./docker/dock.sh logs [service]"
        ;;
    logs)
        docker compose logs --follow $service
        ;;
    status)
        docker compose ps --all
        ;;
    shell)
        if [ -z "$service" ]; then
            echo "Missing the service to open a terminal into." >&2
            display_usage
            exit 1
        fi
        if [ -n "$(docker compose ps --quiet --status running $service)" ]; then
            docker compose exec $service bash
        else
            # Not running, e.g. because its application failed: open a terminal
            # in a fresh container of the same image to find out why.
            echo "$service is not running: opening a terminal into a new container."
            docker compose run --rm --no-deps $service bash
        fi
        ;;
    stop)
        docker compose stop $service
        ;;
    clean)
        docker compose down --rmi all --remove-orphans
        ;;
    *)
        echo "Unknown parameter: $command"
        display_usage
        ;;
esac
