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
#include <optional>
#include <string>

#include <behaviortree_cpp/decorator_node.h>
#include <behaviortree_ros2/ros_node_params.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <stepit_camera_msgs/msg/camera_status.hpp>

namespace stepit_behaviors
{

/**
 * @brief Runs its child with the camera's live view off: a camera in live
 * view ignores a shot.
 *
 * Before the child, it reads `streaming` on the camera's status. When the
 * live view is on, it calls the camera's stop_streaming and waits for its
 * answer, which comes once the camera has left the live view; it fails if the
 * live view cannot be stopped. With `resume`, it switches the live view on
 * again once the child ends, succeeded, failed or halted, without waiting for
 * the answer, as halt() cannot wait; without it, the live view stays off, e.g.
 * after a long stack, so that the camera does not drain its battery in live
 * view.
 *
 * The status is latched and published every second: when none has come yet,
 * it waits for one up to `timeout`, then takes the live view as off.
 */
class PauseLiveView : public BT::DecoratorNode
{
public:
  using CameraStatus = stepit_camera_msgs::msg::CameraStatus;
  using Trigger = std_srvs::srv::Trigger;

  PauseLiveView(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  BT::NodeStatus tick() override;

  void halt() override;

private:
  enum class Phase
  {
    WaitingForStatus,
    Stopping,
    Running,
  };

  /// @brief Ask the camera to switch the live view on again, without waiting, if this node stopped it.
  void resume();

  rclcpp::Logger logger_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::executors::SingleThreadedExecutor executor_;
  rclcpp::Subscription<CameraStatus>::SharedPtr status_subscription_;
  rclcpp::Client<Trigger>::SharedPtr stop_client_;
  rclcpp::Client<Trigger>::SharedPtr start_client_;

  std::optional<bool> streaming_;
  Phase phase_ = Phase::Running;
  std::chrono::steady_clock::time_point deadline_;
  std::optional<rclcpp::Client<Trigger>::FutureAndRequestId> stopping_;
  bool paused_ = false;
};

}  // namespace stepit_behaviors
