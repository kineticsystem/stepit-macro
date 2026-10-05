# The stepit-macro container: the whole rig in one container, the modules and
# the rig's own programs. Ubuntu 24.04 (ROS2 Jazzy) with user "developer".
#
# It holds what the image of each module holds, so that every module builds and
# runs here as in its own container: the packages that the modules' Dockerfiles
# install. The rest of the dependencies are rosdep's, installed by update.sh
# into the image that `dock.sh build` commits, RViz included, which StepIt
# Motors declares.
#
# ros:jazzy-ros-base, not the desktop image of the motors' own container: it is
# published for arm64 too, for the Raspberry Pi 5 that runs the rig, and leaves
# out Gazebo and the desktop tools, gigabytes the rig never uses.
FROM ros:jazzy-ros-base

LABEL name="develop:stepit-macro"

RUN apt-get update
RUN apt-get -y install sudo

# Ubuntu 24.04 base images ship a default 'ubuntu' user at uid/gid 1000. Remove
# it so 'developer' can be created with the uid/gid of the host user (passed in
# via build args), which keeps the bind-mounted repo writable.
RUN if id -u ubuntu >/dev/null 2>&1; then userdel -r ubuntu; fi
RUN if getent group ubuntu >/dev/null 2>&1; then groupdel ubuntu; fi

ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd -g ${USER_GID} developer
RUN useradd --create-home --shell /bin/bash --uid ${USER_UID} --gid ${USER_GID} developer
RUN echo 'developer:developer' | chpasswd              # Update user developer password.
RUN usermod -aG sudo developer                          # Add user to sudoers.

RUN apt-get -y upgrade
RUN apt-get -y install wget unzip git python3 python-is-python3 python3-pip python3-pytest
RUN apt-get install -y python3-colcon-common-extensions python3-vcstool
RUN apt-get install -y --no-install-recommends ros-jazzy-rmw-cyclonedds-cpp
RUN apt-get install -y --no-install-recommends ros-jazzy-launch ros-jazzy-launch-ros
RUN apt-get install -y --no-install-recommends ros-jazzy-ament-cmake-gtest ros-jazzy-ament-cmake-gmock
RUN apt-get install -y --no-install-recommends pre-commit clang-format

# The commander: BehaviorTree.CPP, and rosbridge for the editor.
RUN apt-get install -y --no-install-recommends ros-jazzy-behaviortree-cpp ros-jazzy-rosbridge-suite \
    ros-jazzy-generate-parameter-library libboost-dev libfmt-dev libyaml-cpp-dev
# NOTE: the apt python3-websockets crashes on some ROS2 base images -- it
# still passes the removed `loop=` kwarg to asyncio.Lock() internally.
# Install a current version from PyPI instead, as the commander's image does.
RUN pip3 install --no-cache-dir --break-system-packages "websockets>=12"

# The camera: libgphoto2, the live view, and the web server of its test page.
RUN apt-get install -y --no-install-recommends pkg-config libgphoto2-dev gphoto2 usbutils libjpeg-dev
RUN apt-get install -y --no-install-recommends ros-jazzy-rqt-image-view ros-jazzy-compressed-image-transport
RUN apt-get install -y --no-install-recommends libcpp-httplib-dev

# The editor's native validator, built with CMake against BehaviorTree.CPP.
RUN apt-get install -y --no-install-recommends build-essential cmake

# The web pages of the camera, the Freezer and the editor are built with
# Node.js, from the official binaries, and pnpm, through corepack, which ships
# with Node.js and pins the pnpm version that each page's package.json declares.
# The same version as the modules' images.
ARG NODE_VERSION=24.15.0
RUN apt-get install -y --no-install-recommends curl xz-utils ca-certificates \
 && arch=$(dpkg --print-architecture | sed 's/amd64/x64/') \
 && curl -fsSL https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-${arch}.tar.xz \
  | tar -xJ -C /usr/local --strip-components=1 \
 && corepack enable

# rosdep, called by update.sh, shells out to `sudo apt-get`, and sudo cannot
# ask for a password without a terminal.
RUN echo 'developer ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/developer \
 && chmod 0440 /etc/sudoers.d/developer

# The serial ports of the robot and of the Freezer belong to the group dialout,
# gid 20 on the host as in the image: its members open them without a grant.
RUN usermod -aG dialout developer

CMD ["bash"]

USER developer
RUN mkdir -p /home/developer/.ros
COPY --chown=developer:developer cyclonedds.xml /home/developer/.ros
ENV HOME=/home/developer

# The DDS configuration. ENV rather than the bashrc, so that it also applies to
# non-interactive commands.
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ENV CYCLONEDDS_URI=/home/developer/.ros/cyclonedds.xml

# Answer corepack's download prompt for pnpm, so that non-interactive commands
# do not hang on it.
ENV COREPACK_ENABLE_DOWNLOAD_PROMPT=0

# The repo is mounted at ~/ws (see docker-compose.yml), and the scripts that
# build the whole rig are on the PATH.
ENV PATH="/home/developer/ws/bin:${PATH}"

# The editor's validator, `validate`, checks the rig's objectives by default,
# with the native validator that bin/modules/build.sh builds into the rig's
# build folder. rig.launch.py passes the same to the editor's server.
ENV BEHAVIORS_DIR=/home/developer/ws/src/plugins/stepit_objectives/objectives
ENV BTCPP_VALIDATOR=/home/developer/ws/build/editor-validator/btcpp_validate

COPY --chown=developer:developer bashrc /home/developer/.bashrc.macro
RUN echo "source ~/.bashrc.macro" >> ~/.bashrc

RUN rosdep update

WORKDIR /home/developer/ws
