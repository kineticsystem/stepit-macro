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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <stepit_behaviors/set_joints.hpp>

namespace stepit_behaviors::test
{
namespace
{
const std::vector<double> kInput{ 0.5, 1.0, 2.0 };

/// @brief Tick SetJoints once on kInput, with the given attributes, and return its output.
std::vector<double> setJoints(const std::string& attributes)
{
  BT::BehaviorTreeFactory factory;
  factory.registerNodeType<SetJoints>("SetJoints");
  auto blackboard = BT::Blackboard::create();
  blackboard->set("input", kInput);
  auto tree =
      factory.createTreeFromText(R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree"><SetJoints input="{input}" )"
                                 R"(joint_names="joint1;joint2;joint3" output="{output}" )" +
                                     attributes + R"(/></BehaviorTree></root>)",
                                 blackboard);
  EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::SUCCESS);
  return blackboard->get<std::vector<double>>("output");
}
}  // namespace

TEST(SetJointsNode, ReplacesTheNamedJoints)
{
  EXPECT_EQ(setJoints(R"(joints="joint2" values="3.0")"), (std::vector<double>{ 0.5, 3.0, 2.0 }));
}

TEST(SetJointsNode, OffsetsThemWhenRelative)
{
  EXPECT_EQ(setJoints(R"(joints="joint1;joint3" values="1.0;-1.0" relative="true")"),
            (std::vector<double>{ 1.5, 1.0, 1.0 }));
}

TEST(SetJointsNode, OneValueSetsEveryNamedJoint)
{
  EXPECT_EQ(setJoints(R"(joints="joint1;joint2" values="0.0")"), (std::vector<double>{ 0.0, 0.0, 2.0 }));
}

TEST(SetJointsNode, RefusesAJointItDoesNotKnow)
{
  BT::BehaviorTreeFactory factory;
  factory.registerNodeType<SetJoints>("SetJoints");
  auto blackboard = BT::Blackboard::create();
  blackboard->set("input", kInput);
  auto tree = factory.createTreeFromText(
      R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree"><SetJoints input="{input}" )"
      R"(joint_names="joint1;joint2;joint3" joints="joint9" values="1.0" output="{output}"/></BehaviorTree></root>)",
      blackboard);
  EXPECT_THROW(tree.tickOnce(), BT::RuntimeError);
}

}  // namespace stepit_behaviors::test
