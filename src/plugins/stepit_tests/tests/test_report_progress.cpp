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
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>
#include <stepit_macro_msgs/msg/stack_progress.hpp>

namespace stepit_tests
{
namespace
{
using Progress = stepit_macro_msgs::msg::StackProgress;

// Two reports in one run, as FocusStack makes: none yet, then a picture.
constexpr auto kTwoReports = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="TwoReports">
    <Sequence>
      <ReportProgress done="0" total="3"/>
      <ReportProgress done="2" total="3"/>
    </Sequence>
  </BehaviorTree>
  <BehaviorTree ID="NextRun">
    <ReportProgress done="0" total="5"/>
  </BehaviorTree>
</root>)";
}  // namespace

class ReportProgressTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_report_progress");
    BT::RosNodeParams params;
    params.nh = node_;
    stepit_behaviors::registerNodes(factory_, params);
    factory_.registerBehaviorTreeFromText(kTwoReports);
  }

  /// @brief Run a tree to its end; the tree lives as long as the result.
  BT::Tree run(const std::string& tree)
  {
    auto created = factory_.createTree(tree);
    EXPECT_EQ(created.tickWhileRunning(), BT::NodeStatus::SUCCESS);
    return created;
  }

  /// @brief What a page that subscribes now receives: every sample the topic keeps.
  std::vector<std::pair<uint32_t, uint32_t>> lateSubscriber()
  {
    auto page = std::make_shared<rclcpp::Node>("stepit_tests_report_progress_page");
    std::mutex mutex;
    std::vector<std::pair<uint32_t, uint32_t>> received;
    // A depth above one, so that every latched writer's sample would arrive.
    const auto subscription = page->create_subscription<Progress>(
        "/focus_stack/progress", rclcpp::QoS{ 10 }.reliable().transient_local(), [&](const Progress& message) {
          const std::lock_guard<std::mutex> lock{ mutex };
          received.emplace_back(message.done, message.total);
        });
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(page);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 2 };
    while (std::chrono::steady_clock::now() < deadline)
    {
      executor.spin_some(std::chrono::milliseconds{ 50 });
    }
    const std::lock_guard<std::mutex> lock{ mutex };
    return received;
  }

  rclcpp::Node::SharedPtr node_;
  BT::BehaviorTreeFactory factory_;
};

// One publisher for every ReportProgress: a page that opens during a stack,
// the tree alive, gets the last progress alone, not a sample of each node.
TEST_F(ReportProgressTest, ALateSubscriberGetsTheLastProgressAlone)
{
  const auto tree = run("TwoReports");
  EXPECT_EQ(lateSubscriber(), (std::vector<std::pair<uint32_t, uint32_t>>{ { 2u, 3u } }));
}

// The progress outlives the run, and the next run replaces it.
TEST_F(ReportProgressTest, TheNextRunReplacesTheProgress)
{
  run("TwoReports");  // and its tree ends
  run("NextRun");
  EXPECT_EQ(lateSubscriber(), (std::vector<std::pair<uint32_t, uint32_t>>{ { 0u, 5u } }));
}

}  // namespace stepit_tests
