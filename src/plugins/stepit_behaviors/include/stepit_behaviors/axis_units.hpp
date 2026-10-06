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
 * @brief Converts millimetres of a linear axis into radians of its motor, the
 * unit of every position the objectives send.
 *
 * The ratio is the commander's parameter `mm_per_turn.<joint>`, measured on
 * the robot and set in its parameter file: how far the axis travels per turn
 * of its motor. A positive distance turns the motor the positive way.
 */
class MillimetresToRadians : public BT::SyncActionNode
{
public:
  MillimetresToRadians(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
};

/**
 * @brief Converts degrees of a rotary axis into radians of its motor.
 *
 * The ratio is the commander's parameter `deg_per_turn.<joint>`: how far the
 * axis turns per turn of its motor, 360 divided by its gear ratio. A positive
 * angle turns the motor the positive way.
 */
class DegreesToRadians : public BT::SyncActionNode
{
public:
  DegreesToRadians(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
};

}  // namespace stepit_behaviors
