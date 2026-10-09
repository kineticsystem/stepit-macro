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

// SaveValues and LoadValues, against a fake stack_state, the node that keeps
// the state of the rig.

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_state.hpp"

namespace stepit_tests
{

class Values : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_values");
    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 1000 };
    stepit_behaviors::registerNodes(factory_, params);
  }

  void TearDown() override
  {
    node_.reset();
  }

  /// @brief Tick a tree of one node until it is done.
  BT::NodeStatus run(const std::string& node_xml, const BT::Blackboard::Ptr& blackboard)
  {
    auto tree = factory_.createTreeFromText(
        R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)" + node_xml + R"(</BehaviorTree></root>)", blackboard);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 10 };
    auto status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING && std::chrono::steady_clock::now() < deadline)
    {
      status = tree.tickExactlyOnce();
      std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
    }
    return status;
  }

  BT::NodeStatus save(const std::string& key, const std::vector<double>& values)
  {
    auto blackboard = BT::Blackboard::create();
    blackboard->set("values", values);
    return run(R"(<SaveValues key=")" + key + R"(" values="{values}"/>)", blackboard);
  }

  /// @brief The values loaded, or nothing if LoadValues failed.
  std::optional<std::vector<double>> load(const std::string& key)
  {
    auto blackboard = BT::Blackboard::create();
    if (run(R"(<LoadValues key=")" + key + R"(" values="{values}"/>)", blackboard) != BT::NodeStatus::SUCCESS)
    {
      return std::nullopt;
    }
    return blackboard->get<std::vector<double>>("values");
  }

  rclcpp::Node::SharedPtr node_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(Values, LoadsWhatWasSaved)
{
  FakeState state;
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(load("near"), (std::vector<double>{ 12.5 }));
}

// The state of the rig holds it, for every page to read.
TEST_F(Values, TheStateOfTheRigHoldsWhatWasSaved)
{
  FakeState state;
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(save("far", { 15.0, 1.0 }), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(save("near", { 13.0 }), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(state.get("near"), (std::vector<double>{ 13.0 }));
  EXPECT_EQ(state.get("far"), (std::vector<double>{ 15.0, 1.0 }));
}

// What a page sets, an objective reads.
TEST_F(Values, LoadsWhatAPageSet)
{
  FakeState state{ { { "shots", { 10.0 } } } };
  state.set("shots", { 40.0 });
  EXPECT_EQ(load("shots"), (std::vector<double>{ 40.0 }));
}

// An empty list means not set, e.g. a mark the start of the rig forgot.
TEST_F(Values, FailsForANameNotSetYet)
{
  FakeState state;
  EXPECT_EQ(load("far"), std::nullopt);
}

// The state of the rig knows its names: no other can be saved or read.
TEST_F(Values, FailsForAnUnknownName)
{
  FakeState state;
  EXPECT_EQ(save("nowhere", { 1.0 }), BT::NodeStatus::FAILURE);
  EXPECT_EQ(load("nowhere"), std::nullopt);
}

TEST_F(Values, FailsWhenTheStateRefusesTheValue)
{
  FakeState state;
  state.refuse(true);
  EXPECT_EQ(save("near", { 12.5 }), BT::NodeStatus::FAILURE);
  EXPECT_TRUE(state.get("near").empty());
}

TEST_F(Values, FailsWithoutTheStateOfTheRig)
{
  EXPECT_EQ(save("near", { 12.5 }), BT::NodeStatus::FAILURE);
  EXPECT_EQ(load("near"), std::nullopt);
}

// The commander's node, which BehaviorTree.ROS2 registers everything again
// for after any change of its parameters, holds no state.
TEST_F(Values, TheCommandersNodeHoldsNoState)
{
  FakeState state;
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(node_->has_parameter("state.near"));
}

}  // namespace stepit_tests
