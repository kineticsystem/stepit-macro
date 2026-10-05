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

#include <string>

#include <behaviortree_ros2/bt_action_node.hpp>
#include <rclcpp_action/exceptions.hpp>

namespace stepit_behaviors
{

/**
 * @brief BT::RosActionNode, halted safely when its goal ends at the same time.
 *
 * Halting the node cancels its goal. When the goal has just ended, the action
 * client has already forgotten it, and BehaviorTree.ROS2's cancelGoal() throws
 * rclcpp_action::exceptions::UnknownGoalHandleError, which ends the process:
 * the commander, when another objective replaces the running one at that
 * moment. There is nothing left to cancel then, so the exception is dropped.
 */
template <class ActionT>
class RosActionNode : public BT::RosActionNode<ActionT>
{
public:
  using BT::RosActionNode<ActionT>::RosActionNode;

  void halt() override
  {
    try
    {
      BT::RosActionNode<ActionT>::halt();
    }
    catch (const rclcpp_action::exceptions::UnknownGoalHandleError&)
    {
      RCLCPP_INFO(this->logger(), "%s: halted as its goal ended", this->name().c_str());
      this->onHalt();
    }
  }
};

}  // namespace stepit_behaviors
