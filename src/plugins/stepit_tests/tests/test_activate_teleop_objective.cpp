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

// End to end test of the ActivateTeleop objective: it loads the XML shipped by
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
constexpr auto kObjective = "ActivateTeleop";

// An objective of another robot program, handing the robot to the user.
constexpr auto kCaller = R"(
<root BTCPP_format="4" main_tree_to_execute="HandOver">
  <BehaviorTree ID="HandOver">
    <SubTree ID="ActivateTeleop"/>
  </BehaviorTree>
</root>
)";

}  // namespace

class ActivateTeleopObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_teleop");

    // An objective was moving the robot with the trajectory controller.
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "joint_trajectory_controller", "active", true },
                                                        { "joint_state_broadcaster", "active", false },
                                                        { "velocity_controller", "inactive", true },
                                                        { "position_controller", "inactive", true } });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };

    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "ensure_controllers.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "activate_teleop.xml").string());
  }

  void TearDown() override
  {
    manager_.reset();
    node_.reset();
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeControllerManager> manager_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(ActivateTeleopObjective, TheVelocityControllerReplacesTheRunningOne)
{
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  const auto request = manager_->lastSwitch();
  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->activate_controllers, (std::vector<std::string>{ "velocity_controller" }));
  EXPECT_EQ(request->deactivate_controllers, (std::vector<std::string>{ "joint_trajectory_controller" }));
  EXPECT_EQ(manager_->stateOf("joint_state_broadcaster"), "active");
}

// The gamepad's stop button may be pressed while the gamepad already drives
// the robot: that must not restart the velocity controller.
TEST_F(ActivateTeleopObjective, RunningItTwiceIsHarmless)
{
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);

  const auto request = manager_->lastSwitch();
  ASSERT_TRUE(request.has_value());
  EXPECT_TRUE(request->deactivate_controllers.empty());
  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
}

TEST_F(ActivateTeleopObjective, AnotherObjectiveCanHandTheRobotToTheUser)
{
  factory_.registerBehaviorTreeFromText(kCaller);

  ASSERT_EQ(runObjective(factory_, "HandOver", ""), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
  EXPECT_EQ(manager_->stateOf("joint_trajectory_controller"), "inactive");
}

}  // namespace stepit_tests
