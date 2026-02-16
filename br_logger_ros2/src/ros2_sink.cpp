#include "br_logger_ros2/ros2_sink.hpp"

namespace br_logger
{
namespace ros2
{

ROS2Sink::ROS2Sink(rclcpp::Logger logger) : ros_logger_(std::move(logger)) {}

void ROS2Sink::Write(const LogEntry& entry)
{
  if (!ShouldLog(entry.level))
  {
    return;
  }

  rcutils_log_location_t location;
  location.function_name = entry.function_name;
  location.file_name = entry.file_name;
  location.line_number = entry.line;

  rcutils_log(&location, MapLevel(entry.level), ros_logger_.get_name(), "%s", entry.msg);
}

void ROS2Sink::Flush() {}

int ROS2Sink::MapLevel(LogLevel level)
{
  switch (level)
  {
    case LogLevel::TRACE:
    case LogLevel::DEBUG:
      return RCUTILS_LOG_SEVERITY_DEBUG;
    case LogLevel::INFO:
      return RCUTILS_LOG_SEVERITY_INFO;
    case LogLevel::WARN:
      return RCUTILS_LOG_SEVERITY_WARN;
    case LogLevel::ERROR:
      return RCUTILS_LOG_SEVERITY_ERROR;
    case LogLevel::FATAL:
      return RCUTILS_LOG_SEVERITY_FATAL;
    default:
      return RCUTILS_LOG_SEVERITY_INFO;
  }
}

}  // namespace ros2
}  // namespace br_logger
