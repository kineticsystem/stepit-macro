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

// Tests of the gamepad teleoperation: the sticks against the velocity
// controller's topic, the stop button against a fake commander.

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <stepit_teleop/gamepad_teleop.hpp>

#include "fake/fake_commander.hpp"

namespace stepit_tests
{
namespace
{
using stepit_teleop::JointAxis;
using stepit_teleop::toVelocities;

constexpr auto kStopButton = 1;

/// @brief Wait until the condition holds, or two seconds have passed.
bool waitFor(const std::function<bool()>& condition)
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 2 };
  while (!condition())
  {
    if (std::chrono::steady_clock::now() > deadline)
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  }
  return true;
}

}  // namespace

TEST(ToVelocities, EachJointFollowsItsAxisTimesItsScale)
{
  const std::vector<JointAxis> joints{ { 0, 2.0 }, { 3, -1.0 } };
  EXPECT_EQ(toVelocities({ 0.5F, 0.0F, 0.0F, 1.0F }, joints), (std::vector<double>{ 1.0, -1.0 }));
}

TEST(ToVelocities, AJointWithoutAnAxisIsHeldAtZero)
{
  const std::vector<JointAxis> joints{ { -1, 2.0 }, { 7, 2.0 } };
  EXPECT_EQ(toVelocities({ 1.0F, 1.0F }, joints), (std::vector<double>{ 0.0, 0.0 }));
}

class GamepadTeleopTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    commander_ = std::make_unique<FakeCommander>();

    rclcpp::NodeOptions options;
    options.parameter_overrides({ { "joints", std::vector<std::string>{ "joint1", "joint2" } },
                                  { "joint1.axis", 0 },
                                  { "joint1.scale", 2.0 },
                                  { "joint2.axis", 3 },
                                  { "joint2.scale", -1.0 },
                                  { "stop_button", kStopButton },
                                  { "joy_timeout", 0.3 } });
    teleop_ = std::make_shared<stepit_teleop::GamepadTeleop>(options);

    node_ = std::make_shared<rclcpp::Node>("stepit_tests_gamepad");
    joy_publisher_ = node_->create_publisher<sensor_msgs::msg::Joy>("/joy", 10);
    command_subscription_ = node_->create_subscription<std_msgs::msg::Float64MultiArray>(
        "/velocity_controller/commands", 10, [this](const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
          const std::lock_guard<std::mutex> lock{ mutex_ };
          commands_.push_back(msg->data);
        });
    client_ = rclcpp_action::create_client<FakeCommander::ExecuteTree>(node_, "/commander/execute_objective");

    // Created here, after rclcpp::init: an executor needs the context.
    executor_ = std::make_unique<rclcpp::executors::MultiThreadedExecutor>();
    executor_->add_node(teleop_);
    executor_->add_node(node_);
    spinner_ = std::thread{ [this]() { executor_->spin(); } };

    ASSERT_TRUE(client_->wait_for_action_server(std::chrono::seconds{ 2 }));
    ASSERT_TRUE(waitFor([this]() { return joy_publisher_->get_subscription_count() > 0; }));
    ASSERT_TRUE(waitFor([this]() { return command_subscription_->get_publisher_count() > 0; }));
  }

  void TearDown() override
  {
    // The fake commander does not preempt: end a goal still holding.
    client_->async_cancel_all_goals();
    std::this_thread::sleep_for(std::chrono::milliseconds{ 200 });
    executor_->cancel();
    spinner_.join();
    executor_.reset();
    commander_.reset();
  }

  void sendJoy(std::vector<float> axes, bool stop_pressed = false)
  {
    sensor_msgs::msg::Joy joy;
    joy.axes = std::move(axes);
    joy.buttons = { 0, 0, 0, 0 };
    joy.buttons[kStopButton] = stop_pressed ? 1 : 0;
    joy_publisher_->publish(joy);
  }

  std::vector<std::vector<double>> commands() const
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return commands_;
  }

  /// @brief Whether the commander was asked to hand the robot to the gamepad.
  bool activated() const
  {
    for (const auto& event : commander_->events())
    {
      if (event == "goal ActivateTeleop ")
      {
        return true;
      }
    }
    return false;
  }

  std::unique_ptr<FakeCommander> commander_;
  std::shared_ptr<stepit_teleop::GamepadTeleop> teleop_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;
  rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr command_subscription_;
  rclcpp_action::Client<FakeCommander::ExecuteTree>::SharedPtr client_;

  mutable std::mutex mutex_;
  std::vector<std::vector<double>> commands_;

  std::unique_ptr<rclcpp::executors::MultiThreadedExecutor> executor_;
  std::thread spinner_;
};

TEST_F(GamepadTeleopTest, TheSticksCommandOneVelocityPerJoint)
{
  sendJoy({ 0.5F, 0.0F, 0.0F, 1.0F });

  ASSERT_TRUE(waitFor([this]() { return !commands().empty(); }));
  EXPECT_EQ(commands().back(), (std::vector<double>{ 1.0, -1.0 }));
}

// The velocity controller may be driven by someone else while the gamepad is
// idle: sticks at rest are sent once, not repeated.
TEST_F(GamepadTeleopTest, SticksAtRestAreSentOnce)
{
  sendJoy({ 0.5F, 0.0F, 0.0F, 0.0F });
  ASSERT_TRUE(waitFor([this]() { return commands().size() == 1; }));
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F });
  ASSERT_TRUE(waitFor([this]() { return commands().size() == 2; }));
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F });
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F });

  std::this_thread::sleep_for(std::chrono::milliseconds{ 200 });
  EXPECT_EQ(commands().size(), 2U);
  EXPECT_EQ(commands().back(), (std::vector<double>{ 0.0, 0.0 }));
}

// The commander preempts: the stop button only asks for ActivateTeleop, which
// replaces the running objective. Cancelling it too would be redundant.
TEST_F(GamepadTeleopTest, TheStopButtonRunsActivateTeleop)
{
  FakeCommander::ExecuteTree::Goal hold;
  hold.target_tree = FakeCommander::kHold;
  client_->async_send_goal(hold);
  ASSERT_TRUE(waitFor([this]() { return !commander_->events().empty(); }));

  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F }, true);

  ASSERT_TRUE(waitFor([this]() { return activated(); }));
  EXPECT_EQ(commander_->events(), (std::vector<std::string>{ "goal Hold ", "goal ActivateTeleop " }));
  // The joints are stopped at once, in case the velocity controller is the one running.
  ASSERT_FALSE(commands().empty());
  EXPECT_EQ(commands().front(), (std::vector<double>{ 0.0, 0.0 }));
}

TEST_F(GamepadTeleopTest, TheStopButtonWorksWithNoObjectiveRunning)
{
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F }, true);

  ASSERT_TRUE(waitFor([this]() { return activated(); }));
}

// joy_linux_node repeats its message while a button is held: only the press counts.
TEST_F(GamepadTeleopTest, HoldingTheStopButtonAsksOnce)
{
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F }, true);
  ASSERT_TRUE(waitFor([this]() { return activated(); }));
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F }, true);
  sendJoy({ 0.0F, 0.0F, 0.0F, 0.0F }, true);

  std::this_thread::sleep_for(std::chrono::milliseconds{ 200 });
  EXPECT_EQ(commander_->events().size(), 1U);
}

// A gamepad unplugged with a stick held must not leave the joint turning.
TEST_F(GamepadTeleopTest, TheJointsStopWhenTheGamepadGoesSilent)
{
  sendJoy({ 1.0F, 0.0F, 0.0F, 0.0F });
  ASSERT_TRUE(waitFor([this]() { return !commands().empty(); }));
  EXPECT_EQ(commands().back(), (std::vector<double>{ 2.0, 0.0 }));

  ASSERT_TRUE(waitFor([this]() { return commands().back() == std::vector<double>{ 0.0, 0.0 }; }));
}

}  // namespace stepit_tests
