cmake_minimum_required(VERSION 3.16)
project(stepit_state)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(yaml-cpp REQUIRED)
# yaml-cpp 0.8 exports a namespaced target, older versions only the plain one.
if(TARGET yaml-cpp::yaml-cpp)
  set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
else()
  set(YAML_CPP_TARGET yaml-cpp)
endif()

# The node, as a library, so that the tests can run it on a state file of
# their own.
add_library(stepit_state SHARED src/stack_state.cpp)
target_include_directories(stepit_state PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(stepit_state PUBLIC rclcpp::rclcpp PRIVATE ${YAML_CPP_TARGET})

add_executable(stack_state src/main.cpp)
target_link_libraries(stack_state PRIVATE stepit_state)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS stepit_state
  EXPORT export_stepit_state
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS stack_state DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME}/)

ament_export_targets(export_stepit_state HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp)
ament_package()
