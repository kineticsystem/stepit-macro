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

// End to end test of the ToggleTeleop objective: it loads the XML shipped by
// stepit_objectives and runs it against a fake controller manager.

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <stepit_behaviors/register_nodes.hpp>
#include <rclcpp/rclcpp.hpp>

#include "fake/fake_controller_manager.hpp"
#include "objective.hpp"

namespace stepit_tests
{
namespace
{
constexpr auto kObjective = "ToggleTeleop";
}  // namespace

class ToggleTeleopObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_toggle_teleop");

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };

    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "ensure_controllers.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "activate_teleop.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "toggle_teleop.xml").string());
  }

  void TearDown() override
  {
    manager_.reset();
    node_.reset();
  }

  /** The controllers of the rig, with the given one driving the robot. */
  void driving(const std::string& controller)
  {
    std::vector<FakeControllerManager::Controller> controllers{ { "joint_state_broadcaster", "active", false } };
    for (const auto* name : { "joint_trajectory_controller", "velocity_controller", "position_controller" })
    {
      controllers.push_back({ name, name == controller ? "active" : "inactive", true });
    }
    manager_ = std::make_unique<FakeControllerManager>(controllers);
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeControllerManager> manager_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(ToggleTeleopObjective, HandsTheRobotToTheGamepad)
{
  driving("joint_trajectory_controller");

  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
  EXPECT_EQ(manager_->stateOf("joint_trajectory_controller"), "inactive");
  EXPECT_EQ(manager_->stateOf("joint_state_broadcaster"), "active");
}

// The button stops whatever drives the robot, e.g. the position controller of
// a direct move, not only the trajectory controller.
TEST_F(ToggleTeleopObjective, TakesTheRobotFromAnyController)
{
  driving("position_controller");

  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
  EXPECT_EQ(manager_->stateOf("position_controller"), "inactive");
}

TEST_F(ToggleTeleopObjective, HandsTheRobotBackWhenTheGamepadDrivesIt)
{
  driving("velocity_controller");

  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  const auto request = manager_->lastSwitch();
  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->activate_controllers, (std::vector<std::string>{ "joint_trajectory_controller" }));
  EXPECT_EQ(request->deactivate_controllers, (std::vector<std::string>{ "velocity_controller" }));
  EXPECT_EQ(manager_->stateOf("joint_state_broadcaster"), "active");
}

TEST_F(ToggleTeleopObjective, TwoPressesComeBackToTheStart)
{
  driving("joint_trajectory_controller");

  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("joint_trajectory_controller"), "active");
  EXPECT_EQ(manager_->stateOf("velocity_controller"), "inactive");
}

}  // namespace stepit_tests
