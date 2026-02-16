#include "br_logger_ros2/ros2_context.hpp"

#include <rmw/rmw.h>

#include <br_logger/log_context.hpp>
#include <cstdio>
#include <cstring>

#if defined(__linux__)
#include <unistd.h>
#endif

#ifndef BR_LOG_ROS2_PACKAGE_NAME
#define BR_LOG_ROS2_PACKAGE_NAME "unknown"
#endif

namespace br_logger
{
namespace ros2
{

ROS2ContextProvider::ROS2ContextProvider(rclcpp::Node::SharedPtr node)
{
  auto& ctx = LogContext::Instance();

  ctx.SetGlobalTag("ros.node", node->get_name());
  ctx.SetGlobalTag("ros.namespace", node->get_namespace());

  char domain_buf[16] = {};
  std::snprintf(domain_buf, sizeof(domain_buf), "%zu",
                node->get_node_options().context()->get_domain_id());
  ctx.SetGlobalTag("ros.domain_id", domain_buf);

  ctx.SetGlobalTag("ros.package", BR_LOG_ROS2_PACKAGE_NAME);

#if defined(__linux__)
  char exe_path[256] = {};
  ssize_t len = ::readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (len > 0)
  {
    exe_path[len] = '\0';
    const char* name = exe_path;
    for (const char* p = exe_path; *p != '\0'; ++p)
    {
      if (*p == '/')
      {
        name = p + 1;
      }
    }
    ctx.SetGlobalTag("ros.executable", name);
  }
  else
  {
    ctx.SetGlobalTag("ros.executable", "unknown");
  }
#else
  ctx.SetGlobalTag("ros.executable", "unknown");
#endif

  ctx.SetGlobalTag("ros.rmw", rmw_get_implementation_identifier());
}

}  // namespace ros2
}  // namespace br_logger
