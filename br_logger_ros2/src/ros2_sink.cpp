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
    case LogLevel::Trace:
      return RCUTILS_LOG_SEVERITY_DEBUG;
    case LogLevel::Debug:
      return RCUTILS_LOG_SEVERITY_DEBUG;
    case LogLevel::Info:
      return RCUTILS_LOG_SEVERITY_INFO;
    case LogLevel::Warn:
      return RCUTILS_LOG_SEVERITY_WARN;
    case LogLevel::Error:
      return RCUTILS_LOG_SEVERITY_ERROR;
    case LogLevel::Fatal:
      return RCUTILS_LOG_SEVERITY_FATAL;
    default:
      return RCUTILS_LOG_SEVERITY_INFO;
  }
}

}  // namespace ros2
}  // namespace br_logger
