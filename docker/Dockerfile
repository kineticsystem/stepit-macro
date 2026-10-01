# The stepit-macro container: the rig's own programs, which run next to the
# modules, e.g. the gamepad. Ubuntu 24.04 (ROS2 Jazzy) with user "developer",
# matching the containers of the modules, so that they all discover each other's
# ROS2 graph over DDS on the host network (see cyclonedds.xml).
FROM ros:jazzy-ros-base

LABEL name="develop:stepit-macro"

RUN apt-get update
RUN apt-get -y install sudo

# ros:jazzy-ros-base (Ubuntu 24.04) ships a default 'ubuntu' user at uid/gid
# 1000. Remove it so 'developer' can be created with the uid/gid of the host
# user (passed in via build args), which keeps the bind-mounted repo writable.
RUN if id -u ubuntu >/dev/null 2>&1; then userdel -r ubuntu; fi
RUN if getent group ubuntu >/dev/null 2>&1; then groupdel ubuntu; fi

ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd -g ${USER_GID} developer
RUN useradd --create-home --shell /bin/bash --uid ${USER_UID} --gid ${USER_GID} developer
RUN echo 'developer:developer' | chpasswd              # Update user developer password.
RUN usermod -aG sudo developer                          # Add user to sudoers.

RUN apt-get -y install python3-colcon-common-extensions
RUN apt-get install -y --no-install-recommends ros-jazzy-launch
RUN apt-get install -y --no-install-recommends ros-jazzy-launch-ros
RUN apt-get install -y --no-install-recommends ros-jazzy-rmw-cyclonedds-cpp
RUN apt-get install -y --no-install-recommends ros-jazzy-ament-cmake-gtest
RUN apt-get install -y --no-install-recommends pre-commit clang-format

# rosdep, called by update.sh, shells out to `sudo apt-get`, and sudo cannot
# ask for a password without a terminal.
RUN echo 'developer ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/developer \
 && chmod 0440 /etc/sudoers.d/developer

CMD ["bash"]

USER developer
RUN mkdir -p /home/developer/.ros
COPY --chown=developer:developer cyclonedds.xml /home/developer/.ros
ENV HOME=/home/developer

# The DDS configuration of every container of the rig. ENV rather than the
# bashrc, so that it also applies to non-interactive commands.
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ENV CYCLONEDDS_URI=/home/developer/.ros/cyclonedds.xml

# The repo is mounted at ~/ws (see docker-compose.yml), and the scripts that
# build src/stepit-macro are on the PATH.
ENV PATH="/home/developer/ws/bin/stepit-macro:${PATH}"

COPY --chown=developer:developer bashrc /home/developer/.bashrc.macro
RUN echo "source ~/.bashrc.macro" >> ~/.bashrc

WORKDIR /home/developer/ws
