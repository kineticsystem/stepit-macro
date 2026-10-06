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

#include "stepit_behaviors/is_controller_active.hpp"

#include <algorithm>

namespace stepit_behaviors
{

IsControllerActive::IsControllerActive(const std::string& name, const BT::NodeConfig& config,
                                       const BT::RosNodeParams& params)
  : BT::RosServiceNode<controller_manager_msgs::srv::ListControllers>(name, config, params)
{
}

BT::PortsList IsControllerActive::providedPorts()
{
  return providedBasicPorts({
      BT::InputPort<std::string>("controller", "the controller to check, e.g. velocity_controller"),
  });
}

bool IsControllerActive::setRequest(Request::SharedPtr& /* request */)
{
  // The service takes no argument.
  return true;
}

BT::NodeStatus IsControllerActive::onResponseReceived(const Response::SharedPtr& response)
{
  const auto controller = getInput<std::string>("controller");
  if (!controller)
  {
    RCLCPP_ERROR(logger(), "%s: %s", name().c_str(), controller.error().c_str());
    return BT::NodeStatus::FAILURE;
  }

  const auto& controllers = response->controller;
  const bool active = std::any_of(controllers.cbegin(), controllers.cend(), [&](const auto& state) {
    return state.name == controller.value() && state.state == "active";
  });

  RCLCPP_INFO(logger(), "%s: %s is %s", name().c_str(), controller.value().c_str(), active ? "active" : "not active");

  return active ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

BT::NodeStatus IsControllerActive::onFailure(BT::ServiceNodeErrorCode error)
{
  RCLCPP_ERROR(logger(), "%s: %s", name().c_str(), toStr(error));
  return BT::NodeStatus::FAILURE;
}

}  // namespace stepit_behaviors
