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
#include <std_msgs/msg/string.hpp>

namespace stepit_behaviors
{

/// @brief The file StackDone writes into the folder of a finished stack.
inline constexpr auto kStackDoneFile = "stack.json";

/**
 * @brief Announces that the pictures of one rail's stack are all on disk: a
 * folder that stops growing may only be waiting for its next shot, so the
 * files alone cannot tell.
 *
 * Its folder is the one SetPictureFolder gave the camera, with the same
 * `folder`, `index` and `degrees`, e.g. 2026-10-06_15-20-04/angle_01_-17.0deg,
 * under the camera's folder of pictures, the parameter `pictures_folder`. It
 * runs after the last shot: StepIt Camera reports a picture only once it is
 * saved, and ExpectPicture waits for it, so every picture is there.
 *
 * It does two things:
 *
 * - It writes stack.json into the folder, the shots and the angle of the stack
 *   and the files the folder holds, written to a temporary file and renamed, so
 *   that its presence says the stack is complete, also to whoever reads the
 *   pictures later, without following the topic.
 * - It publishes the folder, relative to the pictures folder, on a latched
 *   topic, /focus_stack/stack_done by default, as a std_msgs/String.
 *
 * It fails neither the stack nor the objective when the file cannot be
 * written: it logs why, and still publishes.
 */
class StackDone : public BT::SyncActionNode
{
public:
  StackDone(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

private:
  std::weak_ptr<rclcpp::Node> node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

}  // namespace stepit_behaviors
