cmake_minimum_required(VERSION 3.16)
project(stepit_bringup)

find_package(ament_cmake REQUIRED)

# Installed as links to the sources by build.sh (--symlink-install): an edit of
# config/rig.yaml applies on the next start, with no build, and the launch file
# finds the repo from where it really is.
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_package()
