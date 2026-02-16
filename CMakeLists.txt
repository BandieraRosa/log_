cmake_minimum_required(VERSION 3.16)
project(br_logger VERSION 1.0.0 LANGUAGES CXX)

option(BR_LOG_BUILD_TESTS "Build unit tests" OFF)

if(BR_LOG_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(br_logger_core)

option(BR_LOG_BUILD_ROS2 "Build ROS2 extension layer" OFF)
if(BR_LOG_BUILD_ROS2)
    add_subdirectory(br_logger_ros2)
endif()

option(BR_LOG_BUILD_EXAMPLES "Build examples" OFF)

if(BR_LOG_BUILD_EXAMPLES)
    add_subdirectory(examples/basic_cpp)
endif()
