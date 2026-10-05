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

#include "stepit_behaviors/shoot.hpp"

namespace stepit_behaviors
{

Shoot::Shoot(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : RosActionNode<freezer_msgs::action::Shoot>(name, config, params)
{
}

BT::PortsList Shoot::providedPorts()
{
  return providedBasicPorts({
      BT::InputPort<std::string>("sequence", "",
                                 "the sequence to fire, as named in the Freezer's parameters; empty for its "
                                 "default_sequence"),
      BT::OutputPort<unsigned>("shot_id", "the id the board gave the shot"),
  });
}

bool Shoot::setGoal(Goal& goal)
{
  goal.sequence = getInput<std::string>("sequence").value_or("");

  const std::string what = goal.sequence.empty() ? "the default sequence" : "sequence '" + goal.sequence + "'";
  RCLCPP_INFO(logger(), "%s: firing %s", name().c_str(), what.c_str());
  return true;
}

BT::NodeStatus Shoot::onResultReceived(const WrappedResult& result)
{
  // An aborted shot never comes here: BehaviorTree.ROS2 calls onFailure with
  // ACTION_ABORTED, without the result. The Freezer node logs why.
  setOutput("shot_id", static_cast<unsigned>(result.result->shot_id));
  RCLCPP_INFO(logger(), "%s: shot %u fired, %u us", name().c_str(), static_cast<unsigned>(result.result->shot_id),
              result.result->duration_us);
  return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus Shoot::onFailure(BT::ActionNodeErrorCode error)
{
  RCLCPP_ERROR(logger(), "%s: %s; the log of the Freezer node says why", name().c_str(), toStr(error));
  return BT::NodeStatus::FAILURE;
}

void Shoot::onHalt()
{
  // The Freezer refuses to cancel a shot that has started: it runs to its end.
  RCLCPP_INFO(logger(), "%s: halted; a shot that has started runs to its end", name().c_str());
}

}  // namespace stepit_behaviors
