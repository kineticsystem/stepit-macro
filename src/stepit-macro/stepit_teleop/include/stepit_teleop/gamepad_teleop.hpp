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
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <btcpp_ros2_interfaces/action/execute_tree.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

namespace stepit_teleop
{

/// @brief The axis of the gamepad that drives one joint, and how fast.
struct JointAxis
{
  /// @brief Index into sensor_msgs/Joy.axes; negative for a joint the gamepad does not drive.
  std::int64_t axis = -1;
  /// @brief Velocity at full deflection, in rad/s; a negative scale reverses the direction.
  double scale = 0.0;
};

/**
 * @brief The velocity of each joint for the given state of the axes.
 *
 * A joint whose axis is negative, or missing from the message, gets 0.
 */
std::vector<double> toVelocities(const std::vector<float>& axes, const std::vector<JointAxis>& joints);

/**
 * @brief Drives the robot with a gamepad.
 *
 * The sticks set the velocity of the joints: each /joy message becomes one
 * command on the velocity controller's topic, one value per joint of the
 * controller, in its order. The commands only move the robot while the
 * velocity controller is active.
 *
 * The stop button sends zero velocities, then asks the commander for the
 * objective of the parameter `objective`, ActivateTeleop by default, which
 * stops every other controller driving the robot and activates the velocity
 * controller. The rig's gamepad runs ToggleTeleop instead, which also hands
 * the robot back when the gamepad already drives it: the node does not know
 * which, the objective decides. The commander preempts: the objective
 * replaces whichever objective is running, which is halted.
 *
 * If /joy goes silent, e.g. because the gamepad was unplugged with a stick
 * held, the node sends zero velocities once.
 */
class GamepadTeleop : public rclcpp::Node
{
public:
  using ExecuteTree = btcpp_ros2_interfaces::action::ExecuteTree;

  explicit GamepadTeleop(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  void onJoy(const sensor_msgs::msg::Joy& msg);
  void onWatchdog();
  void publish(const std::vector<double>& velocities);
  void stop();
  void activateTeleop();

  std::string controller_;
  std::string objective_;
  std::vector<JointAxis> joints_;
  std::int64_t stop_button_;
  std::chrono::nanoseconds joy_timeout_;

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_subscription_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr command_publisher_;
  rclcpp_action::Client<ExecuteTree>::SharedPtr commander_;
  rclcpp::TimerBase::SharedPtr watchdog_;

  std::vector<double> last_velocities_;
  bool stop_button_pressed_ = false;
  std::optional<rclcpp::Time> last_joy_;
  /// @brief When the switch to the velocity controller in progress started, if one is.
  std::optional<rclcpp::Time> switch_started_;
};

}  // namespace stepit_teleop
