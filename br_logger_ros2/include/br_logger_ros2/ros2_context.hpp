#pragma once
#include <rclcpp/rclcpp.hpp>

namespace br_logger
{
namespace ros2
{

class ROS2ContextProvider
{
 public:
  explicit ROS2ContextProvider(rclcpp::Node::SharedPtr node);
};

}  // namespace ros2
}  // namespace br_logger
