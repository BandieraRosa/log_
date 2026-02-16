#include "br_logger_ros2/ros2_bridge.hpp"

#include <br_logger/formatters/pattern_formatter.hpp>
#include <br_logger/logger.hpp>
#include <br_logger/sinks/console_sink.hpp>
#include <br_logger/sinks/rotating_file_sink.hpp>
#include <memory>
#include <string>

#include "br_logger_ros2/ros2_context.hpp"
#include "br_logger_ros2/ros2_sink.hpp"

namespace br_logger
{
namespace ros2
{

void init(rclcpp::Node::SharedPtr node, const BridgeConfig& config)
{
  ROS2ContextProvider ctx_provider(node);

  auto& logger = Logger::Instance();

  if (config.enable_ros2_sink)
  {
    auto sink = std::make_unique<ROS2Sink>(node->get_logger());
    logger.AddSink(std::move(sink));
  }

  if (config.enable_console)
  {
    auto sink = std::make_unique<ConsoleSink>();
    sink->SetFormatter(std::make_unique<PatternFormatter>(config.console_pattern, true));
    logger.AddSink(std::move(sink));
  }

  if (config.enable_file)
  {
    std::string log_path = config.file_path + std::string(node->get_name()) + ".log";
    auto sink = std::make_unique<RotatingFileSink>(log_path, config.max_file_size,
                                                   config.max_files);
    logger.AddSink(std::move(sink));
  }

  logger.Start();
}

void shutdown() { Logger::Instance().Stop(); }

}  // namespace ros2
}  // namespace br_logger
