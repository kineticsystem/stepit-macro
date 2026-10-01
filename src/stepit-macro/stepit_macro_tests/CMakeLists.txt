cmake_minimum_required(VERSION 3.16)
project(stepit_macro_tests)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(btcpp_ros2_interfaces REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(rclcpp_action REQUIRED)
  find_package(sensor_msgs REQUIRED)
  find_package(std_msgs REQUIRED)
  find_package(stepit_teleop REQUIRED)

  # Driving the robot with a gamepad, against a fake commander.
  ament_add_gtest(test_gamepad_teleop tests/test_gamepad_teleop.cpp)
  target_include_directories(test_gamepad_teleop PRIVATE tests)
  target_link_libraries(test_gamepad_teleop
    stepit_teleop::stepit_teleop
    rclcpp::rclcpp
    rclcpp_action::rclcpp_action
    ${btcpp_ros2_interfaces_TARGETS}
    ${sensor_msgs_TARGETS}
    ${std_msgs_TARGETS})
endif()

ament_package()
