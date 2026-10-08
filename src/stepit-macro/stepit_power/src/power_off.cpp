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

#include "stepit_power/power_off.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <utility>

namespace stepit_power
{
namespace
{
/// @brief logind's PowerOff over the system's D-Bus, not interactive: a refusal comes back at once, no prompt.
const std::vector<std::string> kPowerOff{
  "busctl",   "call", "--system", "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
  "PowerOff", "b",    "false"
};

std::string trimmed(const std::string& text)
{
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos)
  {
    return "";
  }
  return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}
}  // namespace

CommandResult runCommand(const std::vector<std::string>& command)
{
  if (command.empty())
  {
    return { -1, "no command" };
  }
  int pipe_ends[2];
  if (::pipe(pipe_ends) != 0)
  {
    return { -1, std::string("cannot run ") + command[0] + ": " + std::strerror(errno) };
  }
  const pid_t child = ::fork();
  if (child < 0)
  {
    ::close(pipe_ends[0]);
    ::close(pipe_ends[1]);
    return { -1, std::string("cannot run ") + command[0] + ": " + std::strerror(errno) };
  }
  if (child == 0)
  {
    // The child: what it prints goes to the pipe, then it becomes the command.
    ::dup2(pipe_ends[1], STDOUT_FILENO);
    ::dup2(pipe_ends[1], STDERR_FILENO);
    ::close(pipe_ends[0]);
    ::close(pipe_ends[1]);
    std::vector<char*> argv;
    for (const auto& part : command)
    {
      argv.push_back(const_cast<char*>(part.c_str()));
    }
    argv.push_back(nullptr);
    ::execvp(argv[0], argv.data());
    const std::string error = std::string("cannot run ") + command[0] + ": " + std::strerror(errno) + "\n";
    const auto written = ::write(STDERR_FILENO, error.data(), error.size());
    static_cast<void>(written);
    ::_exit(127);
  }
  ::close(pipe_ends[1]);
  std::string output;
  char buffer[4096];
  ssize_t count;
  while ((count = ::read(pipe_ends[0], buffer, sizeof(buffer))) > 0)
  {
    output.append(buffer, static_cast<size_t>(count));
  }
  ::close(pipe_ends[0]);
  int status = 0;
  ::waitpid(child, &status, 0);
  return { WIFEXITED(status) ? WEXITSTATUS(status) : -1, output };
}

PowerOff::PowerOff(const rclcpp::NodeOptions& options, RunCommand run)
  : rclcpp::Node("power_off", options), run_(std::move(run))
{
  command_ = declare_parameter<std::vector<std::string>>("command", kPowerOff);
  refuse_during_ = declare_parameter<std::vector<std::string>>("refuse_during", { "FocusStack", "Stack" });
  const auto topic = declare_parameter<std::string>("objective_topic", "/stepit_server/objective");

  // Latched: the objective running reaches this node however late it starts.
  objective_subscription_ = create_subscription<std_msgs::msg::String>(
      topic, rclcpp::QoS(1).reliable().transient_local(), [this](const std_msgs::msg::String& message) {
        const std::lock_guard<std::mutex> lock{ mutex_ };
        objective_ = message.data;
      });
  service_ = create_service<std_srvs::srv::Trigger>(
      "~/power_off",
      [this](const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
             std::shared_ptr<std_srvs::srv::Trigger::Response> response) { onPowerOff(request, response); });
}

void PowerOff::onPowerOff(const std::shared_ptr<std_srvs::srv::Trigger::Request>& /*request*/,
                          const std::shared_ptr<std_srvs::srv::Trigger::Response>& response)
{
  std::string objective;
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    objective = objective_;
  }
  if (std::find(refuse_during_.begin(), refuse_during_.end(), objective) != refuse_during_.end())
  {
    response->success = false;
    response->message = objective + " is running: stop it first";
    RCLCPP_WARN(get_logger(), "Not switching off: %s", response->message.c_str());
    return;
  }
  const auto result = run_(command_);
  if (result.status == 0)
  {
    response->success = true;
    response->message = "Switching off";
    RCLCPP_WARN(get_logger(), "Switching off, as asked");
    return;
  }
  const auto output = trimmed(result.output);
  response->success = false;
  response->message = "The computer refused to switch off: " +
                      (output.empty() ? "exit status " + std::to_string(result.status) : output);
  RCLCPP_ERROR(get_logger(), "%s", response->message.c_str());
}

}  // namespace stepit_power
