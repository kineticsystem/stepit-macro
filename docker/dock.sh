#! /bin/bash -e

# Use this script to download, build, start, stop and remove the whole StepIt
# Macro rig: the robot, the commander, the editor and the camera, each in its
# own container.
#
# The containers are defined in docker-compose.yml, which extends the compose
# file of each module. This script only adds what compose cannot express: the
# submodules, the X server permission, the host uid/gid, and compiling the code
# of each module inside its container.

# The services of docker-compose.yml, in the order they are built and started.
SERVICES=(stepit-driver stepit-commander stepit-editor stepit-camera)

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
    for module in ${SERVICES[@]}; do
        if [ ! -f "../modules/$module/docker/docker-compose.yml" ]; then
            echo "Module '$module' is missing: run ./docker/dock.sh download first." >&2
            exit 1
        fi
    done
}

# Install the dependencies and compile the code of a service, in a throwaway
# container of its image: the code lives in the bind-mounted module, so what
# is built stays on the host for the service's own container to run.
function compile() {
    local service="$1"
    echo "Compiling $service"
    docker compose run --rm --no-deps $service bash -c "update.sh && build.sh"
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
        # Rebuilds only the layers that a Dockerfile changed since last time,
        # so there is no need to clean first.
        docker compose build $service
        for s in ${service:-${SERVICES[@]}}; do
            compile $s
        done
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
