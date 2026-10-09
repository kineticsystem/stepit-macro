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
  find_package(stepit_teleop_msgs REQUIRED)

  # Driving the robot with a gamepad, against a fake commander.
  ament_add_gtest(test_gamepad_teleop tests/test_gamepad_teleop.cpp)
  target_include_directories(test_gamepad_teleop PRIVATE tests)
  target_link_libraries(test_gamepad_teleop
    stepit_teleop::stepit_teleop
    rclcpp::rclcpp
    rclcpp_action::rclcpp_action
    ${btcpp_ros2_interfaces_TARGETS}
    ${sensor_msgs_TARGETS}
    ${std_msgs_TARGETS}
    ${stepit_teleop_msgs_TARGETS})

  # Switching the computer off on request, with a command of the test's own.
  find_package(std_srvs REQUIRED)
  find_package(stepit_power REQUIRED)
  ament_add_gtest(test_power_off tests/test_power_off.cpp)
  target_link_libraries(test_power_off
    stepit_power::stepit_power
    rclcpp::rclcpp
    ${std_msgs_TARGETS}
    ${std_srvs_TARGETS})

  # The state of the rig, on a state file of the test's own.
  find_package(stepit_state REQUIRED)
  ament_add_gtest(test_stack_state tests/test_stack_state.cpp)
  target_link_libraries(test_stack_state
    stepit_state::stepit_state
    rclcpp::rclcpp)

  # The configuration of the rig, rig.yaml, as rig.launch.py reads it.
  find_package(ament_cmake_pytest REQUIRED)
  ament_add_pytest_test(test_rig_config tests/test_rig_config.py)
endif()

ament_package()
