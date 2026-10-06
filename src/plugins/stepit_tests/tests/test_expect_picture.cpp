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

// ExpectPicture around Shoot, against a fake Freezer whose shots release the
// shutter of a fake camera.

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_camera.hpp"
#include "fake/fake_freezer.hpp"

namespace stepit_tests
{
namespace
{
std::string treeXml(const std::string& body)
{
  return R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)" + body + R"(</BehaviorTree></root>)";
}

constexpr auto kShot = R"(<ExpectPicture timeout="1"><Shoot/></ExpectPicture>)";
}  // namespace

class ExpectPictureTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_expect_picture");
    camera_ = std::make_unique<FakeCamera>();
    freezer_ = std::make_unique<FakeFreezer>(std::set<std::string>{ "test_shot" }, "test_shot",
                                             std::chrono::milliseconds{ 50 });
    freezer_->onShot([this]() { camera_->release(); });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
  }

  void TearDown() override
  {
    freezer_.reset();
    camera_.reset();
    node_.reset();
  }

  BT::NodeStatus run(const std::string& body)
  {
    auto tree = factory_.createTreeFromText(treeXml(body));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 10 };
    auto status = BT::NodeStatus::RUNNING;
    while (status == BT::NodeStatus::RUNNING && std::chrono::steady_clock::now() < deadline)
    {
      status = tree.tickExactlyOnce();
      std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
    }
    return status;
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeCamera> camera_;
  std::unique_ptr<FakeFreezer> freezer_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(ExpectPictureTest, SucceedsOnceThePictureComes)
{
  EXPECT_EQ(run(kShot), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(freezer_->fired().size(), 1u);
  EXPECT_EQ(camera_->taken(), 1);
}

// The Freezer fired, but the camera ignored the release.
TEST_F(ExpectPictureTest, FailsWhenNoPictureComes)
{
  camera_->ignore(1);
  EXPECT_EQ(run(kShot), BT::NodeStatus::FAILURE);
  EXPECT_EQ(freezer_->fired().size(), 1u);
}

TEST_F(ExpectPictureTest, ARetryFiresTheShotAgain)
{
  camera_->ignore(1);
  EXPECT_EQ(run(std::string(R"(<RetryUntilSuccessful num_attempts="2">)") + kShot + "</RetryUntilSuccessful>"),
            BT::NodeStatus::SUCCESS);
  EXPECT_EQ(freezer_->fired().size(), 2u);
  EXPECT_EQ(camera_->taken(), 1);
}

// A camera set to RAW+JPEG gives two files a shot: one is not enough.
TEST_F(ExpectPictureTest, WaitsForEveryFileOfTheShot)
{
  EXPECT_EQ(run(R"(<ExpectPicture timeout="1" files="2"><Shoot/></ExpectPicture>)"), BT::NodeStatus::FAILURE);

  freezer_->onShot([this]() {
    camera_->release();
    camera_->release();
  });
  EXPECT_EQ(run(R"(<ExpectPicture timeout="1" files="2"><Shoot/></ExpectPicture>)"), BT::NodeStatus::SUCCESS);
}

TEST_F(ExpectPictureTest, AFailedShotFailsAtOnce)
{
  EXPECT_EQ(run(R"(<ExpectPicture timeout="1"><Shoot sequence="unknown"/></ExpectPicture>)"), BT::NodeStatus::FAILURE);
  EXPECT_EQ(camera_->taken(), 0);
}

}  // namespace stepit_tests
