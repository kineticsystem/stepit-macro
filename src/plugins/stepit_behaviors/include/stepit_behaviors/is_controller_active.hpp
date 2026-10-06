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

#include <behaviortree_ros2/bt_service_node.hpp>
#include <controller_manager_msgs/srv/list_controllers.hpp>

namespace stepit_behaviors
{

/**
 * @brief Tells whether a controller is active: SUCCESS if it is, FAILURE if it
 * is not, or if the controller manager cannot be asked.
 *
 * A condition for an objective that acts by the state of the robot, e.g.
 * ToggleTeleop, which hands the robot back when the gamepad drives it.
 */
class IsControllerActive : public BT::RosServiceNode<controller_manager_msgs::srv::ListControllers>
{
public:
  IsControllerActive(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  bool setRequest(Request::SharedPtr& request) override;

  BT::NodeStatus onResponseReceived(const Response::SharedPtr& response) override;

  BT::NodeStatus onFailure(BT::ServiceNodeErrorCode error) override;
};

}  // namespace stepit_behaviors
