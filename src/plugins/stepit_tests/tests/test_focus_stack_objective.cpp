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

// End to end tests of MarkNear, MarkFar and FocusStack: the XML shipped by
// stepit_objectives, run against a fake robot, a fake controller manager, a
// fake Freezer and a fake camera, with the overshoot of rig.yaml set as the
// commander's parameters.

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_camera.hpp"
#include "fake/fake_controller_manager.hpp"
#include "fake/fake_freezer.hpp"
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
/// The stage, joint1, at 0.5; the rail, joint2, at the near mark.
const std::vector<double> kJointPositions{ 0.5, 1.0, 0.0, -1.0, 2.0 };

constexpr double kStageOvershoot = 0.1;
constexpr double kRailOvershoot = 0.2;

/// 3 shots from 1.0 to 2.0 on the rail, at 2 angles of the stage, 0.5 either side of where it is.
constexpr auto kPayload = "{shots: 3, stage_from: -0.5, stage_to: 0.5, angles: 2}";

/// The stage and the rail of each command, rounded, to compare them.
std::vector<std::pair<double, double>> stageAndRail(const std::vector<std::vector<double>>& commands)
{
  std::vector<std::pair<double, double>> moves;
  for (const auto& command : commands)
  {
    moves.emplace_back(std::round(command[0] * 1000.0) / 1000.0, std::round(command[1] * 1000.0) / 1000.0);
  }
  return moves;
}
}  // namespace

class FocusStackObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    state_file_ = std::filesystem::temp_directory_path() /
                  ("stepit_focus_stack_" + std::string(testing::UnitTest::GetInstance()->current_test_info()->name())) /
                  "stack.yaml";
    std::filesystem::remove_all(state_file_.parent_path());

    // The commander's section of rig.yaml.
    rclcpp::NodeOptions options;
    options.parameter_overrides({ rclcpp::Parameter("state_file", state_file_.string()),
                                  rclcpp::Parameter("overshoot.joint1", kStageOvershoot),
                                  rclcpp::Parameter("overshoot.joint2", kRailOvershoot) });
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_focus_stack", options);

    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);
    robot_->followPositionCommands(kCommandTopic, 100.0);
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "velocity_controller", "active", true },
                                                        { "position_controller", "inactive", true },
                                                        { "joint_state_broadcaster", "active", false } });
    camera_ = std::make_unique<FakeCamera>();
    freezer_ = std::make_unique<FakeFreezer>(std::set<std::string>{ "test_shot" }, "test_shot",
                                             std::chrono::milliseconds{ 20 });
    freezer_->onShot([this]() { camera_->release(); });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
    for (const auto* file : { "ensure_controllers.xml", "mark_near.xml", "mark_far.xml", "focus_stack.xml" })
    {
      factory_.registerBehaviorTreeFromFile(treePath("objectives", file).string());
    }
  }

  void TearDown() override
  {
    freezer_.reset();
    camera_.reset();
    manager_.reset();
    robot_.reset();
    node_.reset();
    std::filesystem::remove_all(state_file_.parent_path());
  }

  /// @brief The marks, as MarkNear and MarkFar would have saved them.
  void mark(double near, double far)
  {
    std::filesystem::create_directories(state_file_.parent_path());
    std::ofstream(state_file_) << "near: [" << near << "]\nfar: [" << far << "]\n";
  }

  /// @brief The values LoadValues reads under `key`, or nothing.
  std::optional<std::vector<double>> saved(const std::string& key)
  {
    auto blackboard = BT::Blackboard::create();
    auto tree = factory_.createTreeFromText(R"(<root BTCPP_format="4"><BehaviorTree ID="Read"><LoadValues key=")" +
                                                key + R"(" values="{values}"/></BehaviorTree></root>)",
                                            blackboard);
    if (tree.tickOnce() != BT::NodeStatus::SUCCESS)
    {
      return std::nullopt;
    }
    return blackboard->get<std::vector<double>>("values");
  }

  std::filesystem::path state_file_;
  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeRobot> robot_;
  std::unique_ptr<FakeControllerManager> manager_;
  std::unique_ptr<FakeCamera> camera_;
  std::unique_ptr<FakeFreezer> freezer_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(FocusStackObjective, TheMarksSaveWhereTheRailIs)
{
  ASSERT_EQ(runObjective(factory_, "MarkNear", ""), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(saved("near"), (std::vector<double>{ 1.0 }));
  EXPECT_EQ(saved("far"), std::nullopt);

  ASSERT_EQ(runObjective(factory_, "MarkFar", ""), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(saved("far"), (std::vector<double>{ 1.0 }));
  EXPECT_EQ(saved("near"), (std::vector<double>{ 1.0 }));
}

// Marking does not switch the controllers: the gamepad keeps driving.
TEST_F(FocusStackObjective, MarkingLeavesTheControllersAlone)
{
  ASSERT_EQ(runObjective(factory_, "MarkNear", ""), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(manager_->lastSwitch().has_value());
  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
}

TEST_F(FocusStackObjective, ShootsAtEveryRailPositionOfEveryAngle)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(freezer_->fired().size(), 6u);
  EXPECT_EQ(camera_->taken(), 6);
  EXPECT_EQ(manager_->stateOf("position_controller"), "active");
}

// Every move approaches its position from the start of the stack: the rail
// from near to far, the stage upward. The stage starts at 0.5, so its angles
// are 0.0 and 1.0.
TEST_F(FocusStackObjective, EveryPositionIsApproachedTheSameWay)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  const std::vector<std::pair<double, double>> expected{
    // To the start: the stage comes down to 0.0, against the approach, and
    // the rail is already at the near mark, maybe driven there either way:
    // both go past, and back.
    { -0.1, 0.8 },
    { 0.0, 1.0 },
    // The first angle: the rail steps toward the far mark, already there for the first shot.
    { 0.0, 1.0 },
    { 0.0, 1.5 },
    { 0.0, 2.0 },
    // The second angle: the stage turns on, with the approach; the rail comes
    // back to the near mark, against it: past, and back.
    { 1.0, 0.8 },
    { 1.0, 1.0 },
    { 1.0, 1.5 },
    { 1.0, 2.0 },
    // Back to the start.
    { 0.5, 1.0 },
  };
  EXPECT_EQ(stageAndRail(robot_->positionCommands()), expected);
}

TEST_F(FocusStackObjective, TheOtherJointsStayInPlace)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  for (const auto& command : robot_->positionCommands())
  {
    EXPECT_DOUBLE_EQ(command[2], 0.0);
    EXPECT_DOUBLE_EQ(command[3], -1.0);
    EXPECT_DOUBLE_EQ(command[4], 2.0);
  }
}

// The camera ignored a release, as the real one sometimes does: the shot is
// fired again, and the stack has every picture.
TEST_F(FocusStackObjective, AShotTheCameraIgnoredIsFiredAgain)
{
  mark(1.0, 2.0);
  camera_->ignore(1);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 90 }), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(freezer_->fired().size(), 7u);
  EXPECT_EQ(camera_->taken(), 6);
}

// Twice in a row: the stack stops rather than leave a gap.
TEST_F(FocusStackObjective, StopsWhenTheCameraIgnoresAShotTwice)
{
  mark(1.0, 2.0);
  camera_->ignore(2);
  EXPECT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 90 }), BT::NodeStatus::FAILURE);

  EXPECT_EQ(freezer_->fired().size(), 2u);
  EXPECT_EQ(camera_->taken(), 0);
}

TEST_F(FocusStackObjective, NeedsBothMarks)
{
  std::filesystem::create_directories(state_file_.parent_path());
  std::ofstream(state_file_) << "near: [1.0]\n";
  EXPECT_EQ(runObjective(factory_, "FocusStack", kPayload), BT::NodeStatus::FAILURE);

  EXPECT_TRUE(robot_->positionCommands().empty());
  EXPECT_TRUE(freezer_->fired().empty());
}

}  // namespace stepit_tests
