cmake_minimum_required(VERSION 3.16)
project(br_logger_ros2_example LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(example_node example_node.cpp)
target_compile_features(example_node PRIVATE cxx_std_17)
target_link_libraries(example_node PRIVATE br_logger_core br_logger_ros2)
ament_target_dependencies(example_node PUBLIC rclcpp std_msgs)

install(TARGETS example_node DESTINATION lib/${PROJECT_NAME})

ament_package()
