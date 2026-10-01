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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <stepit_behaviors/register_nodes.hpp>
#include <rclcpp/rclcpp.hpp>

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
constexpr auto kObjective = "Stack";

const std::vector<std::string> kJointNames{ "joint1", "joint2", "joint3", "joint4", "joint5" };
const std::vector<double> kJointPositions{ 0.5, 1.0, 0.0, -1.0, 2.0 };

/// @brief 5 turns in 10 steps: half a turn each.
constexpr double kStep = M_PI;
}  // namespace

class StackObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_stack");
    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);
    // Fast, so that the 133 moves of the grid take a few seconds.
    robot_->followPositionCommands(kCommandTopic, 400.0);
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "joint_trajectory_controller", "active", true },
                                                        { "position_controller", "inactive", true },
                                                        { "joint_state_broadcaster", "active", false } });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };

    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "ensure_controllers.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "stack.xml").string());
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

// Joint 1 steps half a turn at a time from where it is to 5 turns further,
// and at each of its 11 positions joint 2 does the same from its own start:
// 11 moves of joint 1, each followed by the 11 positions of joint 2, all
// through the position controller.
TEST_F(StackObjective, StepsJoint1AndJoint2ThroughAGrid)
{
  ASSERT_EQ(runObjective(factory_, kObjective, "", std::chrono::seconds{ 120 }), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(manager_->stateOf("position_controller"), "active");
  const auto commands = robot_->positionCommands();
  // The grid, then the way home.
  ASSERT_EQ(commands.size(), 11u * 12u + 1u);

  std::size_t move = 0;
  for (int row = 0; row <= 10; ++row)
  {
    const double joint1 = kJointPositions[0] + row * kStep;
    // Joint 1 moves on, joint 2 goes back to its start, then joint 2 steps.
    for (int shot = -1; shot <= 10; ++shot, ++move)
    {
      const double joint2 = kJointPositions[1] + std::max(shot, 0) * kStep;
      SCOPED_TRACE("row " + std::to_string(row) + ", shot " + std::to_string(shot));
      EXPECT_NEAR(commands[move][0], joint1, 1e-9);
      EXPECT_NEAR(commands[move][1], joint2, 1e-9);
    }
  }
  // Exactly 5 turns from where they started, at the end of the grid.
  const auto& last = commands[11u * 12u - 1u];
  EXPECT_NEAR(last[0], kJointPositions[0] + 10.0 * M_PI, 1e-9);
  EXPECT_NEAR(last[1], kJointPositions[1] + 10.0 * M_PI, 1e-9);
}

// When the grid is done, every joint goes back to where it started.
TEST_F(StackObjective, EveryJointGoesBackHomeAtTheEnd)
{
  ASSERT_EQ(runObjective(factory_, kObjective, "", std::chrono::seconds{ 120 }), BT::NodeStatus::SUCCESS);

  const auto commands = robot_->positionCommands();
  ASSERT_FALSE(commands.empty());
  for (std::size_t i = 0; i < kJointNames.size(); ++i)
  {
    EXPECT_NEAR(commands.back()[i], kJointPositions[i], 1e-9) << kJointNames[i];
    EXPECT_NEAR(robot_->positions()[i], kJointPositions[i], 0.01) << kJointNames[i];
  }
}

// Every move commands all five joints, so joints 3, 4 and 5 are held where
// they were.
TEST_F(StackObjective, Joints3To5StayInPlace)
{
  ASSERT_EQ(runObjective(factory_, kObjective, "", std::chrono::seconds{ 120 }), BT::NodeStatus::SUCCESS);

  const auto commands = robot_->positionCommands();
  ASSERT_FALSE(commands.empty());
  for (const auto& command : commands)
  {
    ASSERT_EQ(command.size(), kJointNames.size());
    EXPECT_DOUBLE_EQ(command[2], kJointPositions[2]);
    EXPECT_DOUBLE_EQ(command[3], kJointPositions[3]);
    EXPECT_DOUBLE_EQ(command[4], kJointPositions[4]);
  }
}

// Cancelling the grid stops the robot where it is, by deactivating the
// position controller, and does not send it home.
TEST_F(StackObjective, CancellingStopsTheRobotWithoutGoingHome)
{
  auto global_blackboard = BT::Blackboard::create();
  auto tree = factory_.createTree(kObjective, BT::Blackboard::create(global_blackboard));
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 30 };
  while (robot_->positionCommands().size() < 5 && std::chrono::steady_clock::now() < deadline)
  {
    ASSERT_EQ(tree.tickExactlyOnce(), BT::NodeStatus::RUNNING);
    std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  }
  ASSERT_GE(robot_->positionCommands().size(), 5u);

  tree.haltTree();

  const auto stopped = std::chrono::steady_clock::now() + std::chrono::seconds{ 2 };
  while (manager_->stateOf("position_controller") != "inactive" && std::chrono::steady_clock::now() < stopped)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  }
  EXPECT_EQ(manager_->stateOf("position_controller"), "inactive");
  EXPECT_LT(robot_->positionCommands().size(), 11u * 12u + 1u);
}

}  // namespace stepit_tests
