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

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <behaviortree_cpp/bt_factory.h>
#include <stepit_behaviors/register_nodes.hpp>
#include <rclcpp/rclcpp.hpp>

#include "fake/fake_robot.hpp"
#include "objective.hpp"

namespace stepit_tests
{
namespace
{
constexpr auto kJointStateTopic = "/joint_states";
constexpr auto kActionName = "/joint_trajectory_controller/follow_joint_trajectory";

const std::vector<std::string> kJointNames{ "joint1", "joint2", "joint3", "joint4", "joint5" };
const std::vector<double> kJointPositions{ 0.5, 1.0, 0.0, -1.0, 2.0 };

// A move through the trajectory controller: no objective of the rig makes one,
// the behaviors are kept for a synchronised or a timed move.
constexpr auto kTree = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="FollowCubic">
    <Sequence>
      <CubicTrajectory joint_names="joint1;joint2" positions="0.0;1.57" duration="0.5" trajectory="{trajectory}"/>
      <FollowJointTrajectory action_name="/joint_trajectory_controller/follow_joint_trajectory"
                             trajectory="{trajectory}"/>
    </Sequence>
  </BehaviorTree>
</root>)";
}  // namespace

class FollowJointTrajectoryTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_follow_joint_trajectory");
    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromText(kTree);
  }

  void TearDown() override
  {
    robot_.reset();
    node_.reset();
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeRobot> robot_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(FollowJointTrajectoryTest, SendsTheTrajectoryToTheController)
{
  ASSERT_EQ(runObjective(factory_, "FollowCubic", ""), BT::NodeStatus::SUCCESS);

  const auto trajectory = robot_->lastTrajectory();
  ASSERT_TRUE(trajectory.has_value());
  EXPECT_EQ(trajectory->joint_names, (std::vector<std::string>{ "joint1", "joint2" }));
  ASSERT_EQ(trajectory->points.size(), 1u);
  EXPECT_EQ(trajectory->points[0].positions, (std::vector<double>{ 0.0, 1.57 }));
}

TEST_F(FollowJointTrajectoryTest, FailsWhenTheControllerFails)
{
  robot_->failNextTrajectory();
  EXPECT_EQ(runObjective(factory_, "FollowCubic", ""), BT::NodeStatus::FAILURE);
}

}  // namespace stepit_tests
