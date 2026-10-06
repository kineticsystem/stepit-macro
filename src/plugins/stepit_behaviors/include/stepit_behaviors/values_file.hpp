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

namespace stepit_behaviors
{

/**
 * @brief Saves numbers under a name in a YAML file, e.g. where the user marked
 * the near end of a stack, for LoadValues to read back in a later objective.
 *
 * The file is the commander's parameter `state_file`, unless the port `file`
 * names another; it holds one list per name, e.g. `near: [12.4]`, and keeps the
 * other names. It is written to a temporary file first, then renamed, so that
 * a crash never leaves it half written. It is state, not configuration: it
 * belongs outside git.
 */
class SaveValues : public BT::SyncActionNode
{
public:
  SaveValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
};

/**
 * @brief Reads the numbers that SaveValues saved under a name. Fails, saying
 * so, when the file or the name is missing, e.g. when the user has not marked
 * that end of the stack yet.
 */
class LoadValues : public BT::SyncActionNode
{
public:
  LoadValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
};

}  // namespace stepit_behaviors
