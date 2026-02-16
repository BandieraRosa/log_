#pragma once
#include <cstddef>
#include <cstdint>
#include <rclcpp/rclcpp.hpp>
#include <string>

namespace br_logger
{
namespace ros2
{

struct BridgeConfig
{
  bool enable_ros2_sink = true;
  bool enable_console = true;
  bool enable_file = false;
  std::string file_path = "/tmp/robot_logs/";
  size_t max_file_size = 50 * 1024 * 1024;
  uint32_t max_files = 5;
  std::string console_pattern = "[%D %T%e] [%C%L%R] [%g] [%f:%#::%n] %m";
};

void init(rclcpp::Node::SharedPtr node, const BridgeConfig& config = {});
void shutdown();

}  // namespace ros2
}  // namespace br_logger
