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
#include <thread>
#include <vector>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_camera.hpp"

namespace stepit_tests
{
namespace
{
// RestorePictureFolder around a child that sets a folder, then ends one way or another.
constexpr auto kTrees = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Succeeds">
    <RestorePictureFolder>
      <SetPictureFolder folder="tests"/>
    </RestorePictureFolder>
  </BehaviorTree>
  <BehaviorTree ID="Fails">
    <RestorePictureFolder>
      <Sequence>
        <SetPictureFolder folder="tests"/>
        <AlwaysFailure/>
      </Sequence>
    </RestorePictureFolder>
  </BehaviorTree>
  <BehaviorTree ID="Runs">
    <RestorePictureFolder>
      <Sequence>
        <SetPictureFolder folder="tests"/>
        <Sleep msec="10000"/>
      </Sequence>
    </RestorePictureFolder>
  </BehaviorTree>
</root>)";
}  // namespace

class RestorePictureFolderTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    camera_ = std::make_unique<FakeCamera>();
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_restore_picture_folder");
    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromText(kTrees);
  }

  void TearDown() override
  {
    camera_.reset();
    node_.reset();
  }

  /// @brief A tree, created early: RestorePictureFolder finds the camera's service meanwhile.
  BT::Tree create(const std::string& id)
  {
    auto tree = factory_.createTree(id);
    std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
    return tree;
  }

  std::unique_ptr<FakeCamera> camera_;
  rclcpp::Node::SharedPtr node_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(RestorePictureFolderTest, AfterAChildThatSucceeds)
{
  auto tree = create("Succeeds");
  EXPECT_EQ(tree.tickWhileRunning(), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(camera_->folders(2), (std::vector<std::string>{ "tests", "" }));
}

TEST_F(RestorePictureFolderTest, AfterAChildThatFails)
{
  auto tree = create("Fails");
  EXPECT_EQ(tree.tickWhileRunning(), BT::NodeStatus::FAILURE);
  EXPECT_EQ(camera_->folders(2), (std::vector<std::string>{ "tests", "" }));
}

// Stop halts the tree: no node is ticked, the decorator restores the folder from halt().
TEST_F(RestorePictureFolderTest, AfterAChildThatIsHalted)
{
  auto tree = create("Runs");
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 5 };
  while (camera_->folders().empty() && std::chrono::steady_clock::now() < deadline)
  {
    ASSERT_EQ(tree.tickExactlyOnce(), BT::NodeStatus::RUNNING);
    std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
  }
  tree.haltTree();
  EXPECT_EQ(camera_->folders(2), (std::vector<std::string>{ "tests", "" }));
}

// Halted before it ever ran, it has nothing to restore.
TEST_F(RestorePictureFolderTest, NotWhenHaltedBeforeItRan)
{
  auto tree = create("Runs");
  tree.haltTree();
  EXPECT_TRUE(camera_->folders(1, std::chrono::milliseconds{ 500 }).empty());
}

}  // namespace stepit_tests
