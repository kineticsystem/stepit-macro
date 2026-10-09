// Copyright 2026 Giovanni Remigi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stepit_behaviors/report_progress.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{

namespace
{
/// @brief A number of pictures: rounded, and never below 0.
uint32_t count(double value)
{
  return static_cast<uint32_t>(std::max(0L, std::lround(value)));
}
}  // namespace

ReportProgress::ReportProgress(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList ReportProgress::providedPorts()
{
  return {
    BT::InputPort<std::string>("topic_name", "/focus_stack/progress", "the latched topic of the progress"),
    BT::InputPort<BT::AnyTypeAllowed>("done", "how much is done, e.g. the pictures taken"),
    BT::InputPort<BT::AnyTypeAllowed>("total", "of how much"),
  };
}

BT::NodeStatus ReportProgress::tick()
{
  if (!publisher_)
  {
    const auto topic = getInput<std::string>("topic_name").value_or("/focus_stack/progress");
    const auto node = node_.lock();
    if (!node)
    {
      throw BT::RuntimeError("ReportProgress: the ROS node went out of scope");
    }
    // Latched: the last value reaches a page that subscribes later.
    publisher_ = node->create_publisher<stepit_macro_msgs::msg::StackProgress>(
        topic, rclcpp::QoS{ 1 }.reliable().transient_local());
  }
  stepit_macro_msgs::msg::StackProgress message;
  message.done = count(requireNumbers(*this, "done").front());
  message.total = count(requireNumbers(*this, "total").front());
  publisher_->publish(message);
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
