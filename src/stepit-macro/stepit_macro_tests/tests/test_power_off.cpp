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

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <stepit_power/power_off.hpp>

namespace stepit_tests
{
namespace
{
using stepit_power::CommandResult;
using stepit_power::PowerOff;
using stepit_power::runCommand;
using Trigger = std_srvs::srv::Trigger;

constexpr auto kObjectiveTopic = "/stepit_tests/objective";
}  // namespace

// The real way of running a command, with harmless ones: never the command
// that switches the computer off.
TEST(RunCommand, GivesTheExitStatusAndWhatItPrinted)
{
  const auto result = runCommand({ "sh", "-c", "echo out; echo err >&2; exit 3" });
  EXPECT_EQ(result.status, 3);
  EXPECT_NE(result.output.find("out"), std::string::npos) << result.output;
  EXPECT_NE(result.output.find("err"), std::string::npos) << result.output;
}

TEST(RunCommand, SaysWhenTheProgramDoesNotExist)
{
  const auto result = runCommand({ "stepit-no-such-program" });
  EXPECT_EQ(result.status, 127);
  EXPECT_NE(result.output.find("cannot run stepit-no-such-program"), std::string::npos) << result.output;
}

/// @brief power_off, with a command of the test's own: it records what would have run, and answers as told.
class PowerOffTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    rclcpp::NodeOptions options;
    options.parameter_overrides(
        { { "objective_topic", kObjectiveTopic }, { "command", std::vector<std::string>{ "switch", "off" } } });
    power_off_ = std::make_shared<PowerOff>(options, [this](const std::vector<std::string>& command) {
      const std::lock_guard<std::mutex> lock{ mutex_ };
      commands_.push_back(command);
      return answer_;
    });
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_power_off");
    objective_ =
        node_->create_publisher<std_msgs::msg::String>(kObjectiveTopic, rclcpp::QoS(1).reliable().transient_local());
    client_ = node_->create_client<Trigger>("/power_off/power_off");
    executor_ = std::make_unique<rclcpp::executors::MultiThreadedExecutor>();
    executor_->add_node(power_off_);
    executor_->add_node(node_);
    spinner_ = std::thread([this]() { executor_->spin(); });
    ASSERT_TRUE(client_->wait_for_service(std::chrono::seconds{ 5 }));
  }

  void TearDown() override
  {
    executor_->cancel();
    spinner_.join();
  }

  /// @brief The commander says this objective runs, and power_off has heard it.
  void running(const std::string& objective)
  {
    std_msgs::msg::String message;
    message.data = objective;
    objective_->publish(message);
    std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
  }

  Trigger::Response press()
  {
    auto future = client_->async_send_request(std::make_shared<Trigger::Request>());
    EXPECT_EQ(future.wait_for(std::chrono::seconds{ 5 }), std::future_status::ready);
    return *future.get();
  }

  std::vector<std::vector<std::string>> commands()
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return commands_;
  }

  std::mutex mutex_;
  std::vector<std::vector<std::string>> commands_;
  CommandResult answer_{ 0, "" };
  std::shared_ptr<PowerOff> power_off_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr objective_;
  rclcpp::Client<Trigger>::SharedPtr client_;
  std::unique_ptr<rclcpp::executors::MultiThreadedExecutor> executor_;
  std::thread spinner_;
};

TEST_F(PowerOffTest, SwitchesOffWhenNothingRuns)
{
  running("");
  const auto response = press();
  EXPECT_TRUE(response.success);
  EXPECT_EQ(response.message, "Switching off");
  EXPECT_EQ(commands(), (std::vector<std::vector<std::string>>{ { "switch", "off" } }));
}

// A stack cut short would leave a session half shot.
TEST_F(PowerOffTest, RefusesWhileAStackRuns)
{
  for (const auto* stack : { "FocusStack", "Stack" })
  {
    running(stack);
    const auto response = press();
    EXPECT_FALSE(response.success) << stack;
    EXPECT_EQ(response.message, std::string(stack) + " is running: stop it first");
  }
  EXPECT_TRUE(commands().empty());
}

TEST_F(PowerOffTest, SwitchesOffWhileAnotherObjectiveRuns)
{
  running("ActivateTeleop");
  EXPECT_TRUE(press().success);
  EXPECT_EQ(commands().size(), 1u);
}

// The system says why it does not switch off, e.g. without the polkit rule.
TEST_F(PowerOffTest, PassesTheSystemsRefusalOn)
{
  running("");
  answer_ = { 1, "Call failed: Access denied\n" };
  const auto response = press();
  EXPECT_FALSE(response.success);
  EXPECT_EQ(response.message, "The computer refused to switch off: Call failed: Access denied");
}

TEST_F(PowerOffTest, SaysTheExitStatusWhenTheCommandSaysNothing)
{
  running("");
  answer_ = { 4, "" };
  EXPECT_EQ(press().message, "The computer refused to switch off: exit status 4");
}

}  // namespace stepit_tests
