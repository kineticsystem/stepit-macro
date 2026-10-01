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

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <btcpp_ros2_interfaces/action/execute_tree.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

namespace stepit_tests
{

/**
 * @brief A stand-in for the commander: an ExecuteTree action server.
 *
 * It records what it is asked, in order: "goal <tree> <payload>" for a goal,
 * "cancel" for a cancel. A goal for the tree "Hold" runs until it is cancelled,
 * like a long objective; any other goal succeeds at once. It does not preempt:
 * the real commander's preemption is tested in stepit-commander.
 */
class FakeCommander
{
public:
  using ExecuteTree = btcpp_ros2_interfaces::action::ExecuteTree;
  using GoalHandle = rclcpp_action::ServerGoalHandle<ExecuteTree>;

  static constexpr auto kHold = "Hold";

  FakeCommander() : node_{ std::make_shared<rclcpp::Node>("fake_commander") }
  {
    server_ = rclcpp_action::create_server<ExecuteTree>(
        node_, "/commander/execute_objective",
        [this](const rclcpp_action::GoalUUID&, std::shared_ptr<const ExecuteTree::Goal> goal) {
          record("goal " + goal->target_tree + " " + goal->payload);
          return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        },
        [this](const std::shared_ptr<GoalHandle>&) {
          record("cancel");
          return rclcpp_action::CancelResponse::ACCEPT;
        },
        [this](const std::shared_ptr<GoalHandle> handle) {
          const std::lock_guard<std::mutex> lock{ mutex_ };
          workers_.emplace_back([handle]() { execute(handle); });
        });

    executor_.add_node(node_);
    spinner_ = std::thread{ [this]() { executor_.spin(); } };
  }

  ~FakeCommander()
  {
    executor_.cancel();
    if (spinner_.joinable())
    {
      spinner_.join();
    }
    for (auto& worker : workers_)
    {
      worker.join();
    }
    executor_.remove_node(node_);
  }

  FakeCommander(const FakeCommander&) = delete;
  FakeCommander& operator=(const FakeCommander&) = delete;

  /// @brief Everything the commander was asked, in order.
  std::vector<std::string> events() const
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return events_;
  }

private:
  static void execute(const std::shared_ptr<GoalHandle>& handle)
  {
    auto result = std::make_shared<ExecuteTree::Result>();
    if (handle->get_goal()->target_tree == kHold)
    {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{ 10 };
      while (!handle->is_canceling() && rclcpp::ok() && std::chrono::steady_clock::now() < deadline)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
      }
      if (handle->is_canceling())
      {
        handle->canceled(result);
        return;
      }
      handle->abort(result);
      return;
    }
    handle->succeed(result);
  }

  void record(const std::string& event)
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    events_.push_back(event);
  }

  rclcpp::Node::SharedPtr node_;
  rclcpp_action::Server<ExecuteTree>::SharedPtr server_;

  mutable std::mutex mutex_;
  std::vector<std::string> events_;
  std::vector<std::thread> workers_;

  rclcpp::executors::SingleThreadedExecutor executor_;
  std::thread spinner_;
};

}  // namespace stepit_tests
