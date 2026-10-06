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

#include <optional>
#include <string>

#include <behaviortree_cpp/action_node.h>
#include <behaviortree_ros2/bt_service_node.hpp>
#include <rcl_interfaces/srv/set_parameters.hpp>

namespace stepit_behaviors
{

/**
 * @brief The local time, as text, e.g. 2026-10-06_15-20-04: a name for a
 * folder of pictures that sorts by time, unique to the second.
 */
class CurrentTime : public BT::SyncActionNode
{
public:
  CurrentTime(const std::string& name, const BT::NodeConfig& config);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;
};

/**
 * @brief The folder of an angle of a stack, inside the stack's folder: its
 * number from 1, two digits, and its angle in degrees, e.g. angle_01_-17.0deg.
 */
std::string angleFolder(int index, std::optional<double> degrees);

/**
 * @brief Sets where the camera saves the next pictures: the parameter `folder`
 * of StepIt Camera, a subfolder of its download_directory, through its
 * set_parameters service.
 *
 * With `index`, the pictures go into the folder of that angle of a stack,
 * inside `folder`, see angleFolder, with `degrees` in its name if given. An
 * empty `folder` sends them to download_directory itself.
 */
class SetPictureFolder : public BT::RosServiceNode<rcl_interfaces::srv::SetParameters>
{
public:
  SetPictureFolder(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  bool setRequest(Request::SharedPtr& request) override;

  BT::NodeStatus onResponseReceived(const Response::SharedPtr& response) override;

  BT::NodeStatus onFailure(BT::ServiceNodeErrorCode error) override;

private:
  std::string folder_;
};

}  // namespace stepit_behaviors
