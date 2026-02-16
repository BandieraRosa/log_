#pragma once
#include <br_logger/log_context.hpp>

#define LOG_SUB_CALLBACK(topic_name)       \
  LOG_SCOPED_TAG("ros.topic", topic_name); \
  LOG_SCOPED_TAG("ros.cb_type", "subscription")

#define LOG_SRV_CALLBACK(service_name)         \
  LOG_SCOPED_TAG("ros.service", service_name); \
  LOG_SCOPED_TAG("ros.cb_type", "service")

#define LOG_TIMER_CALLBACK(timer_name)     \
  LOG_SCOPED_TAG("ros.timer", timer_name); \
  LOG_SCOPED_TAG("ros.cb_type", "timer")

#define LOG_ACTION_CALLBACK(action_name)     \
  LOG_SCOPED_TAG("ros.action", action_name); \
  LOG_SCOPED_TAG("ros.cb_type", "action")
