#include <br_logger/logger.hpp>
#include <br_logger_ros2/ros2_bridge.hpp>
#include <br_logger_ros2/ros2_macros.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class ExampleNode : public rclcpp::Node
{
 public:
  ExampleNode() : Node("example_logger_node") {}

  void Setup()
  {
    br_logger::ros2::BridgeConfig config;
    config.enable_ros2_sink = true;
    config.enable_console = true;
    config.enable_file = true;
    config.file_path = "/tmp/robot_logs/";
    br_logger::ros2::init(shared_from_this(), config);

    br_logger::Logger::Instance().SetLevel(br_logger::LogLevel::Trace);

    sub_ = create_subscription<std_msgs::msg::String>(
        "/chatter", 10, std::bind(&ExampleNode::OnChatter, this, std::placeholders::_1));

    timer_ = create_wall_timer(std::chrono::seconds(2),
                               std::bind(&ExampleNode::OnTimer, this));

    LOG_INFO("node initialized, waiting for messages on /chatter");
  }

  ~ExampleNode() override { br_logger::ros2::shutdown(); }

 private:
  void OnChatter(const std::shared_ptr<const std_msgs::msg::String>& msg)
  {
    LOG_SUB_CALLBACK("/chatter");
    LOG_DEBUG("received: %s", msg->data.c_str());

    if (msg->data.empty())
    {
      LOG_WARN("empty message received");
    }
  }

  void OnTimer()
  {
    LOG_TIMER_CALLBACK("heartbeat");
    LOG_TRACE("timer tick");

    static int count = 0;
    ++count;
    LOG_EVERY_N(Info, 5, "heartbeat count: %d", count);
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<ExampleNode>();
  node->Setup();

  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}
