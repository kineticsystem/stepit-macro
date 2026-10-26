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

// End to end test of TestMotors: the XML shipped by stepit_objectives, with the
// real behaviors, against a fake robot that follows the commands of a position
// controller.

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_controller_manager.hpp"
#include "fake/fake_robot.hpp"
#include "objective.hpp"

namespace stepit_tests
{
namespace
{
constexpr auto kJointStateTopic = "/joint_states";
constexpr auto kActionName = "/joint_trajectory_controller/follow_joint_trajectory";
constexpr auto kCommandTopic = "/position_controller/commands";

const std::vector<std::string> kJointNames{ "joint1", "joint2", "joint3", "joint4", "joint5" };
const std::vector<double> kJointPositions{ 0.5, 1.0, 0.0, -1.0, 2.0 };

/// 720 degrees, counter-clockwise.
constexpr double kTwoTurns = 4.0 * M_PI;
}  // namespace

class TestMotorsObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests");
    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);
    robot_->followPositionCommands(kCommandTopic, 100.0);
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "joint_trajectory_controller", "active", true },
                                                        { "joint_state_broadcaster", "active", false },
                                                        { "position_controller", "inactive", true } });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };

    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "ensure_controllers.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "test_motors.xml").string());
  }

  void TearDown() override
  {
    manager_.reset();
    robot_.reset();
    node_.reset();
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeRobot> robot_;
  std::unique_ptr<FakeControllerManager> manager_;
  BT::BehaviorTreeFactory factory_;
};

// Each motor in turn goes two turns counter-clockwise, then back, while the
// others stay where they are: two commands per motor, from joint1 to joint5.
TEST_F(TestMotorsObjective, TurnsEachMotorTwiceCounterClockwiseAndBack)
{
  ASSERT_EQ(runObjective(factory_, "TestMotors", "{}"), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("position_controller"), "active");
  EXPECT_EQ(manager_->stateOf("joint_trajectory_controller"), "inactive");

  const auto commands = robot_->positionCommands();
  ASSERT_EQ(commands.size(), 2 * kJointNames.size());
  for (std::size_t joint = 0; joint < kJointNames.size(); ++joint)
  {
    auto turned = kJointPositions;
    turned[joint] += kTwoTurns;
    const auto& there = commands[2 * joint];
    const auto& back = commands[2 * joint + 1];
    ASSERT_EQ(there.size(), kJointPositions.size());
    ASSERT_EQ(back.size(), kJointPositions.size());
    for (std::size_t i = 0; i < kJointPositions.size(); ++i)
    {
      EXPECT_NEAR(there[i], turned[i], 0.02) << kJointNames[joint] << " there, joint" << i + 1;
      EXPECT_NEAR(back[i], kJointPositions[i], 0.02) << kJointNames[joint] << " back, joint" << i + 1;
    }
  }
  for (std::size_t i = 0; i < kJointPositions.size(); ++i)
  {
    EXPECT_NEAR(robot_->positions()[i], kJointPositions[i], 0.01) << "joint" << i + 1;
  }
}

}  // namespace stepit_tests
