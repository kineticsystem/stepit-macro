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

#include <behaviortree_cpp/decorator_node.h>
#include <behaviortree_ros2/ros_node_params.hpp>
#include <rcl_interfaces/srv/set_parameters.hpp>
#include <rclcpp/rclcpp.hpp>

namespace stepit_behaviors
{

/**
 * @brief Runs its child, then sends the camera's next pictures to its pictures
 * folder again, however the child ends: succeeded, failed, or halted.
 *
 * An objective that sets the camera's folder, e.g. to the folder of an angle
 * of a stack, wraps that part in it, so that a later picture, from the
 * Freezer's remote trigger or the camera's own shutter, never lands among the
 * stack's. A halt, by Stop or by another objective, ticks no node: this node
 * resets the folder from halt() itself.
 *
 * It sets the parameter `folder` of StepIt Camera to "" through the service of
 * `service_name`, without waiting for the answer, as halt() cannot wait. The
 * reset is therefore not confirmed: if the camera is not there, the folder
 * stays as it was, and the objective does not fail for it. The client is made
 * when the tree is created, so that it has found the service by the time the
 * child ends.
 */
class RestorePictureFolder : public BT::DecoratorNode
{
public:
  RestorePictureFolder(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

  void halt() override;

private:
  /// @brief Ask the camera for the folder "", without waiting.
  void restore();

  rclcpp::Logger logger_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr client_;
};

}  // namespace stepit_behaviors
