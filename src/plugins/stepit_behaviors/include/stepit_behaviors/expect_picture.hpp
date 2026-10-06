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

#include <chrono>
#include <memory>
#include <string>

#include <behaviortree_cpp/decorator_node.h>
#include <behaviortree_ros2/ros_node_params.hpp>
#include <rclcpp/rclcpp.hpp>
#include <stepit_camera_msgs/msg/picture.hpp>

namespace stepit_behaviors
{

/**
 * @brief Runs its child, a shot, then waits until the camera reports the
 * picture: it fails when no picture comes within `timeout`.
 *
 * The Freezer fires the camera through a wire and cannot tell whether the
 * shutter opened: the camera may ignore the release. StepIt Camera publishes
 * every picture it downloads on /camera/picture, so the picture is the proof
 * of the shot. Wrapped in RetryUntilSuccessful, a missed shot is fired again
 * instead of leaving a gap in a stack.
 *
 * It listens from its first tick, before its child runs, so that a picture
 * cannot come unseen. A shot of a camera set to RAW+JPEG gives two files: set
 * `files` to 2, or the second file would count for the next shot.
 */
class ExpectPicture : public BT::DecoratorNode
{
public:
  ExpectPicture(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;
  void halt() override;

private:
  using Picture = stepit_camera_msgs::msg::Picture;

  void connect(const std::string& topic);

  std::weak_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::executors::SingleThreadedExecutor executor_;
  rclcpp::Subscription<Picture>::SharedPtr subscription_;

  bool listening_{ false };
  bool shot_{ false };
  int pictures_{ 0 };
  int files_{ 1 };
  std::chrono::steady_clock::time_point deadline_;
};

}  // namespace stepit_behaviors
