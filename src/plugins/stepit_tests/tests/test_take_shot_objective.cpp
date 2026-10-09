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

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <stepit_behaviors/register_nodes.hpp>
#include <rclcpp/rclcpp.hpp>

#include "fake/fake_camera.hpp"
#include "fake/fake_freezer.hpp"
#include "objective.hpp"

namespace stepit_tests
{
namespace
{
constexpr auto kObjective = "TakeShot";
}  // namespace

class TakeShotObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_take_shot");
    // The camera, whose folder of pictures the objective sets first.
    camera_ = std::make_unique<FakeCamera>();

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 1000 };

    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromFile(treePath("objectives", "take_shot.xml").string());
  }

  void TearDown() override
  {
    freezer_.reset();
    camera_.reset();
    node_.reset();
  }

  void startFreezer()
  {
    freezer_ = std::make_unique<FakeFreezer>(std::set<std::string>{ "test_shot", "timed_light" }, "test_shot");
    freezer_->onShot([this]() { camera_->release(); });
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeCamera> camera_;
  std::unique_ptr<FakeFreezer> freezer_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(TakeShotObjective, WithoutASequenceItFiresTheDefaultOne)
{
  startFreezer();
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(freezer_->fired(), (std::vector<std::string>{ "test_shot" }));
}

// A test shot goes apart from the stacks, into the folder tests.
TEST_F(TakeShotObjective, ThePictureGoesIntoTheFolderTests)
{
  startFreezer();
  ASSERT_EQ(runObjective(factory_, kObjective, ""), BT::NodeStatus::SUCCESS);
  // Then the next pictures go into the pictures folder itself again.
  EXPECT_EQ(camera_->folders(), (std::vector<std::string>{ "tests", "" }));
  EXPECT_EQ(camera_->pictures(), (std::vector<std::string>{ "tests/IMG_1.CR2" }));
}

TEST_F(TakeShotObjective, ItFiresTheSequenceOfThePayload)
{
  startFreezer();
  ASSERT_EQ(runObjective(factory_, kObjective, "{sequence: timed_light}"), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(freezer_->fired(), (std::vector<std::string>{ "timed_light" }));
}

// The Freezer fired, but the camera ignored the release: no picture proves the
// shot, so it fails, as a shot of a stack does.
TEST_F(TakeShotObjective, ItFailsWhenTheCameraIgnoresTheShot)
{
  startFreezer();
  camera_->ignore(1);
  EXPECT_EQ(runObjective(factory_, kObjective, "", std::chrono::seconds{ 30 }), BT::NodeStatus::FAILURE);
  EXPECT_EQ(freezer_->fired(), (std::vector<std::string>{ "test_shot" }));
  EXPECT_EQ(camera_->taken(), 0);
  EXPECT_EQ(camera_->folders(), (std::vector<std::string>{ "tests", "" }));
}

TEST_F(TakeShotObjective, AnUnknownSequenceFails)
{
  startFreezer();
  EXPECT_EQ(runObjective(factory_, kObjective, "{sequence: flash_shot}"), BT::NodeStatus::FAILURE);
  EXPECT_TRUE(freezer_->fired().empty());
}

TEST_F(TakeShotObjective, WithoutTheFreezerItFails)
{
  EXPECT_EQ(runObjective(factory_, kObjective, "", std::chrono::seconds{ 10 }), BT::NodeStatus::FAILURE);
}

// The commander halts an objective that another one replaces: the shot, which
// the Freezer refuses to cancel, still runs to its end.
TEST_F(TakeShotObjective, AHaltedShotRunsToItsEnd)
{
  freezer_ = std::make_unique<FakeFreezer>(std::set<std::string>{ "test_shot" }, "test_shot",
                                           std::chrono::milliseconds{ 500 });
  auto tree = factory_.createTree(kObjective);

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 5 };
  while (freezer_->fired().empty() && std::chrono::steady_clock::now() < deadline)
  {
    ASSERT_EQ(tree.tickExactlyOnce(), BT::NodeStatus::RUNNING);
    std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  }
  ASSERT_EQ(freezer_->fired(), (std::vector<std::string>{ "test_shot" }));

  tree.haltTree();
  EXPECT_EQ(tree.rootNode()->status(), BT::NodeStatus::IDLE);
}

}  // namespace stepit_tests
