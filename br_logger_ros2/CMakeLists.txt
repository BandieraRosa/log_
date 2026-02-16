cmake_minimum_required(VERSION 3.16)
project(br_logger_ros2 VERSION 1.0.0 LANGUAGES CXX)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcutils REQUIRED)
find_package(rmw REQUIRED)

add_library(br_logger_ros2
    src/ros2_context.cpp
    src/ros2_sink.cpp
    src/ros2_bridge.cpp
)

target_include_directories(br_logger_ros2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(br_logger_ros2 PUBLIC cxx_std_17)
target_link_libraries(br_logger_ros2 PUBLIC br_logger_core)
ament_target_dependencies(br_logger_ros2 PUBLIC rclcpp rcutils rmw)

if(DEFINED BR_LOG_ROS2_PACKAGE_NAME)
    target_compile_definitions(br_logger_ros2 PUBLIC
        BR_LOG_ROS2_PACKAGE_NAME="${BR_LOG_ROS2_PACKAGE_NAME}"
    )
endif()

install(TARGETS br_logger_ros2
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/br_logger_ros2/
    DESTINATION include/br_logger_ros2
)

ament_package()
