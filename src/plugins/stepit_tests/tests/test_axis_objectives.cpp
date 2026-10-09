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

// End to end tests of MoveRailBy and RotateStageBy: the XML shipped by
// stepit_objectives, against a fake robot, with the ratios of the axes set as
// the commander's parameters, as rig.yaml sets them.

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

/// 2 mm per turn: 1 mm is half a turn, pi radians.
constexpr double kMmPerTurn = 2.0;
/// 90 degrees per turn: 45 degrees is half a turn, pi radians.
constexpr double kDegPerTurn = 90.0;
}  // namespace

class AxisObjectives : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);
    robot_->followPositionCommands(kCommandTopic, 100.0);
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "joint_trajectory_controller", "active", true },
                                                        { "position_controller", "inactive", true },
                                                        { "joint_state_broadcaster", "active", false } });
  }

  void TearDown() override
  {
    manager_.reset();
    robot_.reset();
    node_.reset();
  }

  /// @brief The behaviors and the objective, on a commander node with these parameters.
  void load(const std::vector<rclcpp::Parameter>& parameters)
  {
    rclcpp::NodeOptions options;
    options.parameter_overrides(parameters);
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_axis_objectives", options);
    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "ensure_controllers.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "move_rail_by.xml").string());
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "rotate_stage_by.xml").string());
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeRobot> robot_;
  std::unique_ptr<FakeControllerManager> manager_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(AxisObjectives, MovesTheRailByMillimetres)
{
  load({ rclcpp::Parameter("mm_per_turn.joint2", kMmPerTurn) });
  ASSERT_EQ(runObjective(factory_, "MoveRailBy", "{mm: 3.0}"), BT::NodeStatus::SUCCESS);

  const auto commands = robot_->positionCommands();
  ASSERT_EQ(commands.size(), 1u);
  EXPECT_NEAR(commands[0][1], 1.0 + 1.5 * 2.0 * M_PI, 1e-9);
  EXPECT_NEAR(robot_->positions()[1], 1.0 + 3.0 * M_PI, 0.01);
}

TEST_F(AxisObjectives, ANegativeDistanceMovesTheOtherWay)
{
  load({ rclcpp::Parameter("mm_per_turn.joint2", kMmPerTurn) });
  ASSERT_EQ(runObjective(factory_, "MoveRailBy", "{mm: -1}"), BT::NodeStatus::SUCCESS);

  EXPECT_NEAR(robot_->positions()[1], 1.0 - M_PI, 0.01);
}

TEST_F(AxisObjectives, TheOtherJointsStayInPlace)
{
  load({ rclcpp::Parameter("mm_per_turn.joint2", kMmPerTurn) });
  ASSERT_EQ(runObjective(factory_, "MoveRailBy", "{mm: 3.0}"), BT::NodeStatus::SUCCESS);

  const auto command = robot_->positionCommands().at(0);
  EXPECT_DOUBLE_EQ(command[0], 0.5);
  EXPECT_DOUBLE_EQ(command[2], 0.0);
  EXPECT_DOUBLE_EQ(command[3], -1.0);
  EXPECT_DOUBLE_EQ(command[4], 2.0);
}

// Without the measured ratio, nothing moves.
TEST_F(AxisObjectives, NeedsTheRatioOfTheRail)
{
  load({});
  EXPECT_NE(runObjective(factory_, "MoveRailBy", "{mm: 3.0}"), BT::NodeStatus::SUCCESS);
  EXPECT_TRUE(robot_->positionCommands().empty());
}

TEST_F(AxisObjectives, TurnsTheStageByDegrees)
{
  load({ rclcpp::Parameter("deg_per_turn.joint1", kDegPerTurn) });
  ASSERT_EQ(runObjective(factory_, "RotateStageBy", "{deg: -45}"), BT::NodeStatus::SUCCESS);

  const auto command = robot_->positionCommands().at(0);
  EXPECT_NEAR(command[0], 0.5 - M_PI, 1e-9);
  EXPECT_DOUBLE_EQ(command[1], 1.0);
  EXPECT_NEAR(robot_->positions()[0], 0.5 - M_PI, 0.01);
}

TEST_F(AxisObjectives, NeedsTheRatioOfTheStage)
{
  load({ rclcpp::Parameter("mm_per_turn.joint2", kMmPerTurn) });
  EXPECT_NE(runObjective(factory_, "RotateStageBy", "{deg: 10}"), BT::NodeStatus::SUCCESS);
  EXPECT_TRUE(robot_->positionCommands().empty());
}

}  // namespace stepit_tests
