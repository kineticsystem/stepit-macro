cmake_minimum_required(VERSION 3.16)
project(stepit_teleop)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

find_package(ament_cmake REQUIRED)
find_package(btcpp_ros2_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(stepit_teleop_msgs REQUIRED)

######################################################
# The node, as a library, so that the tests can create it.

add_library(stepit_teleop SHARED src/gamepad_teleop.cpp)

target_include_directories(stepit_teleop PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(stepit_teleop PUBLIC
  rclcpp::rclcpp
  rclcpp_action::rclcpp_action
  ${btcpp_ros2_interfaces_TARGETS}
  ${sensor_msgs_TARGETS}
  ${std_msgs_TARGETS}
  ${stepit_teleop_msgs_TARGETS})

add_executable(gamepad_teleop src/main.cpp)
target_link_libraries(gamepad_teleop PRIVATE stepit_teleop)

######################################################
# INSTALL

install(DIRECTORY include/ DESTINATION include)

install(TARGETS stepit_teleop
  EXPORT export_stepit_teleop
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

install(TARGETS gamepad_teleop DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_export_targets(export_stepit_teleop HAS_LIBRARY_TARGET)
ament_export_dependencies(
  btcpp_ros2_interfaces
  rclcpp
  rclcpp_action
  sensor_msgs
  std_msgs
  stepit_teleop_msgs)

ament_package()
