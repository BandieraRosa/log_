#pragma once
#include <rcutils/logging.h>

#include <br_logger/sinks/sink_interface.hpp>
#include <rclcpp/rclcpp.hpp>

namespace br_logger
{
namespace ros2
{

class ROS2Sink : public br_logger::ILogSink
{
 public:
  explicit ROS2Sink(rclcpp::Logger logger);

  void Write(const LogEntry& entry) override;
  void Flush() override;

 private:
  rclcpp::Logger ros_logger_;

  static int MapLevel(LogLevel level);
};

}  // namespace ros2
}  // namespace br_logger
