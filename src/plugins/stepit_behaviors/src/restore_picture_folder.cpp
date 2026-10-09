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

#include "stepit_behaviors/restore_picture_folder.hpp"

#include <memory>
#include <string>

#include <rcl_interfaces/msg/parameter.hpp>
#include <rcl_interfaces/msg/parameter_type.hpp>

namespace stepit_behaviors
{

RestorePictureFolder::RestorePictureFolder(const std::string& name, const BT::NodeConfig& config,
                                           const BT::RosNodeParams& params)
  : BT::DecoratorNode(name, config), logger_(rclcpp::get_logger("RestorePictureFolder"))
{
  const auto node = params.nh.lock();
  if (!node)
  {
    throw BT::RuntimeError("RestorePictureFolder: the ROS node went out of scope");
  }
  logger_ = node->get_logger();
  const auto service = getInput<std::string>("service_name").value_or("/camera/set_parameters");
  // A callback group no executor spins: the answers are not wanted, and the
  // commander's executor is left alone.
  callback_group_ = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
  client_ = node->create_client<rcl_interfaces::srv::SetParameters>(service, rclcpp::ServicesQoS(), callback_group_);
}

BT::PortsList RestorePictureFolder::providedPorts()
{
  return {
    BT::InputPort<std::string>("service_name", "/camera/set_parameters",
                               "the set_parameters service of the camera, whose parameter folder is reset"),
  };
}

BT::NodeStatus RestorePictureFolder::tick()
{
  const auto status = child_node_->executeTick();
  if (status == BT::NodeStatus::SUCCESS || status == BT::NodeStatus::FAILURE)
  {
    resetChild();
    restore();
  }
  return status;
}

void RestorePictureFolder::halt()
{
  // Only a child that was running can have left a folder set since the last reset.
  const bool running = status() == BT::NodeStatus::RUNNING;
  BT::DecoratorNode::halt();
  if (running)
  {
    restore();
  }
}

void RestorePictureFolder::restore()
{
  if (!client_->service_is_ready())
  {
    RCLCPP_WARN(logger_, "RestorePictureFolder: %s is not there, the camera keeps its folder",
                client_->get_service_name());
    return;
  }
  auto request = std::make_shared<rcl_interfaces::srv::SetParameters::Request>();
  rcl_interfaces::msg::Parameter folder;
  folder.name = "folder";
  folder.value.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
  folder.value.string_value = "";
  request->parameters.push_back(folder);
  // Nobody reads the answers: forget the requests sent before this one.
  client_->prune_pending_requests();
  client_->async_send_request(request);
  RCLCPP_INFO(logger_, "RestorePictureFolder: the next pictures go into the pictures folder again");
}

}  // namespace stepit_behaviors
