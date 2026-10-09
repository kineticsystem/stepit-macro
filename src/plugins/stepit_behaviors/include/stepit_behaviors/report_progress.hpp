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

#pragma once

#include <memory>
#include <string>

#include <behaviortree_cpp/action_node.h>
#include <behaviortree_ros2/ros_node_params.hpp>
#include <rclcpp/rclcpp.hpp>
#include <stepit_macro_msgs/msg/stack_progress.hpp>

namespace stepit_behaviors
{

/// @brief The topic of a running stack's progress.
inline constexpr auto kProgressTopic = "/focus_stack/progress";

using ProgressPublisher = rclcpp::Publisher<stepit_macro_msgs::msg::StackProgress>::SharedPtr;

/**
 * @brief A latched publisher of progress: the last message reaches a
 * subscriber that comes later.
 *
 * registerNodes creates the publisher of ReportProgress once, when the plugin
 * loads, and gives it to every node it builds, as it does for StackDone. One
 * publisher per node would make each its own latched writer: FocusStack has
 * two, so a page that subscribes during a stack would get a sample of each,
 * the first one's [0, total] among them, in no defined order.
 */
ProgressPublisher progressPublisher(rclcpp::Node& node, const std::string& topic);

/**
 * @brief Publishes how far an objective is, e.g. the pictures a stack took of
 * how many, as a stepit_macro_msgs/StackProgress, on a latched topic: a page that subscribes
 * while the objective runs gets the last value at once, so every page shows
 * the same, whichever started it.
 *
 * It publishes with `publisher`, see progressPublisher, unless `topic_name`
 * names another topic: it then makes a publisher of its own, on its first
 * tick. The last value stays on the topic after the objective ends: a page
 * knows that it ended from the commander, not from here.
 */
class ReportProgress : public BT::SyncActionNode
{
public:
  ReportProgress(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params,
                 ProgressPublisher publisher = nullptr);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
  ProgressPublisher publisher_;
};

}  // namespace stepit_behaviors
