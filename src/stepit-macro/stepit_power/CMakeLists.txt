cmake_minimum_required(VERSION 3.16)
project(stepit_power)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# The node, as a library, so that it can be tested with a command of the
# test's own instead of switching the computer off.
add_library(stepit_power SHARED src/power_off.cpp)
target_include_directories(stepit_power PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(stepit_power PUBLIC
  rclcpp::rclcpp
  ${std_msgs_TARGETS}
  ${std_srvs_TARGETS})

add_executable(power_off src/main.cpp)
target_link_libraries(power_off PRIVATE stepit_power)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS stepit_power
  EXPORT export_stepit_power
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS power_off DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME}/)

ament_export_targets(export_stepit_power HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp std_msgs std_srvs)
ament_package()
