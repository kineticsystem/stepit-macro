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

#include "stepit_teleop/gamepad_teleop.hpp"

#include <algorithm>
#include <stdexcept>

namespace stepit_teleop
{
namespace
{
constexpr auto kJoyTopic = "/joy";
constexpr auto kCommanderAction = "/commander/execute_objective";

/// @brief How long a switch may take before the stop button is allowed to ask again.
constexpr std::chrono::seconds kSwitchTimeout{ 5 };

bool isMoving(const std::vector<double>& velocities)
{
  return std::any_of(velocities.cbegin(), velocities.cend(), [](double v) { return v != 0.0; });
}

}  // namespace

std::vector<double> toVelocities(const std::vector<float>& axes, const std::vector<JointAxis>& joints)
{
  std::vector<double> velocities;
  velocities.reserve(joints.size());
  for (const auto& joint : joints)
  {
    const bool driven = joint.axis >= 0 && static_cast<std::size_t>(joint.axis) < axes.size();
    velocities.push_back(driven ? joint.scale * static_cast<double>(axes[static_cast<std::size_t>(joint.axis)]) : 0.0);
  }
  return velocities;
}

GamepadTeleop::GamepadTeleop(const rclcpp::NodeOptions& options) : rclcpp::Node("gamepad_teleop", options)
{
  controller_ = declare_parameter<std::string>("controller", "velocity_controller");
  objective_ = declare_parameter<std::string>("objective", "ActivateTeleop");
  stop_button_ = declare_parameter<std::int64_t>("stop_button", 0);
  joy_timeout_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double>{ declare_parameter<double>("joy_timeout", 0.5) });

  // One entry per joint of the controller, in the order of its configuration:
  // the controller takes one value per joint, by position.
  const auto joint_names = declare_parameter<std::vector<std::string>>(
      "joints", std::vector<std::string>{ "joint1", "joint2", "joint3", "joint4", "joint5" });
  if (joint_names.empty())
  {
    throw std::invalid_argument("the parameter 'joints' lists no joint");
  }
  for (const auto& name : joint_names)
  {
    JointAxis joint;
    joint.axis = declare_parameter<std::int64_t>(name + ".axis", -1);
    joint.scale = declare_parameter<double>(name + ".scale", 0.0);
    joints_.push_back(joint);
  }
  last_velocities_.assign(joints_.size(), 0.0);

  command_publisher_ = create_publisher<std_msgs::msg::Float64MultiArray>("/" + controller_ + "/commands", 10);
  joy_subscription_ = create_subscription<sensor_msgs::msg::Joy>(
      kJoyTopic, 10, [this](const sensor_msgs::msg::Joy::SharedPtr msg) { onJoy(*msg); });
  commander_ = rclcpp_action::create_client<ExecuteTree>(this, kCommanderAction);
  watchdog_ = create_wall_timer(std::chrono::milliseconds{ 100 }, [this]() { onWatchdog(); });
}

void GamepadTeleop::onJoy(const sensor_msgs::msg::Joy& msg)
{
  last_joy_ = now();

  // Act on the press only, not for as long as the button is held.
  const bool pressed = stop_button_ >= 0 && static_cast<std::size_t>(stop_button_) < msg.buttons.size() &&
                       msg.buttons[static_cast<std::size_t>(stop_button_)] != 0;
  const bool just_pressed = pressed && !stop_button_pressed_;
  stop_button_pressed_ = pressed;
  if (just_pressed)
  {
    stop();
    return;
  }

  // Sticks at rest are sent once, not repeated: the velocity controller may be
  // driven by someone else while the gamepad is idle.
  const auto velocities = toVelocities(msg.axes, joints_);
  if (isMoving(velocities) || velocities != last_velocities_)
  {
    publish(velocities);
  }
}

void GamepadTeleop::onWatchdog()
{
  if (isMoving(last_velocities_) && last_joy_ && now() - *last_joy_ > rclcpp::Duration{ joy_timeout_ })
  {
    RCLCPP_WARN(get_logger(), "No message on %s for %.1f s: stopping the joints", kJoyTopic,
                std::chrono::duration<double>{ joy_timeout_ }.count());
    publish(std::vector<double>(joints_.size(), 0.0));
  }
}

void GamepadTeleop::publish(const std::vector<double>& velocities)
{
  std_msgs::msg::Float64MultiArray command;
  command.data = velocities;
  command_publisher_->publish(command);
  last_velocities_ = velocities;
}

void GamepadTeleop::stop()
{
  // Stops the joints at once if the velocity controller is the one running.
  publish(std::vector<double>(joints_.size(), 0.0));

  if (switch_started_ && now() - *switch_started_ < rclcpp::Duration{ kSwitchTimeout })
  {
    RCLCPP_INFO(get_logger(), "Already running %s", objective_.c_str());
    return;
  }
  if (!commander_->action_server_is_ready())
  {
    RCLCPP_ERROR(get_logger(), "The commander is not running: cannot run %s", objective_.c_str());
    return;
  }

  RCLCPP_INFO(get_logger(), "Stop: running %s", objective_.c_str());
  switch_started_ = now();
  // The commander preempts: the objective replaces the running one, if any.
  activateTeleop();
}

void GamepadTeleop::activateTeleop()
{
  ExecuteTree::Goal goal;
  goal.target_tree = objective_;

  rclcpp_action::Client<ExecuteTree>::SendGoalOptions options;
  options.goal_response_callback = [this](const auto& handle) {
    if (!handle)
    {
      RCLCPP_ERROR(get_logger(), "The commander rejected %s", objective_.c_str());
      switch_started_.reset();
    }
  };
  options.result_callback = [this](const rclcpp_action::ClientGoalHandle<ExecuteTree>::WrappedResult& result) {
    switch_started_.reset();
    if (result.code == rclcpp_action::ResultCode::SUCCEEDED)
    {
      RCLCPP_INFO(get_logger(), "%s succeeded", objective_.c_str());
    }
    else
    {
      RCLCPP_ERROR(get_logger(), "%s failed: %s", objective_.c_str(),
                   result.result ? result.result->return_message.c_str() : "no result");
    }
  };
  commander_->async_send_goal(goal, options);
}

}  // namespace stepit_teleop
