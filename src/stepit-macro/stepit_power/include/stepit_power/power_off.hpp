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

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>

namespace stepit_power
{

/// @brief How a command ended: its exit status, 0 for success, and what it printed.
struct CommandResult
{
  int status;
  std::string output;
};

/// @brief Runs a command, its program and arguments, without a shell, and waits for it to end.
using RunCommand = std::function<CommandResult(const std::vector<std::string>&)>;

/// @brief Runs a command with fork and exec, and returns its exit status and what it printed, stdout and stderr.
CommandResult runCommand(const std::vector<std::string>& command);

/**
 * @brief Switches the computer off, on request: the service `~/power_off`,
 * std_srvs/Trigger, e.g. from a page's power button.
 *
 * It refuses while an objective of `refuse_during` runs, FocusStack and Stack
 * by default, as the commander publishes it on `objective_topic`: switching
 * off would leave a stack half shot. Otherwise it runs `command`, by default
 * busctl asking systemd-logind to power off over the system's D-Bus, which
 * stops every service cleanly, this rig among them. The answer says whether
 * the computer is switching off, or why not: what the command printed, e.g.
 * that the system denied it.
 *
 * logind decides who may power off. The rig's user needs a polkit rule that
 * allows it, see the README.
 */
class PowerOff : public rclcpp::Node
{
public:
  explicit PowerOff(const rclcpp::NodeOptions& options = rclcpp::NodeOptions(), RunCommand run = runCommand);

private:
  void onPowerOff(const std::shared_ptr<std_srvs::srv::Trigger::Request>& request,
                  const std::shared_ptr<std_srvs::srv::Trigger::Response>& response);

  RunCommand run_;
  std::vector<std::string> command_;
  std::vector<std::string> refuse_during_;
  std::mutex mutex_;
  std::string objective_;  ///< The objective running, as the commander last said: empty when none.
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr objective_subscription_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr service_;
};

}  // namespace stepit_power
