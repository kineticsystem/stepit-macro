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
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <freezer_msgs/action/shoot.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

namespace stepit_tests
{

/**
 * @brief A stand-in for the node of the StepIt Freezer board.
 *
 * It serves the action /freezer/shoot as the real node does: a goal names a
 * sequence, empty for the default one, and a sequence it does not know aborts
 * with a message. A shot lasts `duration`, and, as on the board, cannot be
 * cancelled once started. It records the sequences it was asked to fire.
 */
class FakeFreezer
{
public:
  using Shoot = freezer_msgs::action::Shoot;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Shoot>;

  explicit FakeFreezer(std::set<std::string> sequences, std::string default_sequence,
                       std::chrono::milliseconds duration = std::chrono::milliseconds{ 200 })
    : node_{ std::make_shared<rclcpp::Node>("fake_freezer") }
    , sequences_{ std::move(sequences) }
    , default_sequence_{ std::move(default_sequence) }
    , duration_{ duration }
  {
    server_ = rclcpp_action::create_server<Shoot>(
        node_, "/freezer/shoot",
        [](const rclcpp_action::GoalUUID&, std::shared_ptr<const Shoot::Goal>) {
          return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        },
        // A shot that has started always runs to the end.
        [](std::shared_ptr<GoalHandle>) { return rclcpp_action::CancelResponse::REJECT; },
        [this](std::shared_ptr<GoalHandle> goal_handle) {
          const std::lock_guard<std::mutex> lock{ mutex_ };
          shots_.emplace_back([this, goal_handle]() { fire(goal_handle); });
        });

    executor_.add_node(node_);
    spinner_ = std::thread{ [this]() { executor_.spin(); } };
  }

  ~FakeFreezer()
  {
    std::vector<std::thread> shots;
    {
      const std::lock_guard<std::mutex> lock{ mutex_ };
      shots.swap(shots_);
    }
    for (auto& shot : shots)
    {
      shot.join();
    }
    executor_.cancel();
    if (spinner_.joinable())
    {
      spinner_.join();
    }
    executor_.remove_node(node_);
  }

  FakeFreezer(const FakeFreezer&) = delete;
  FakeFreezer& operator=(const FakeFreezer&) = delete;

  /// @brief The sequences fired so far, the default one by its name.
  std::vector<std::string> fired() const
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return fired_;
  }

private:
  void fire(const std::shared_ptr<GoalHandle>& goal_handle)
  {
    const auto& requested = goal_handle->get_goal()->sequence;
    const auto sequence = requested.empty() ? default_sequence_ : requested;
    auto result = std::make_shared<Shoot::Result>();
    if (sequences_.count(sequence) == 0)
    {
      result->message = "Unknown sequence '" + sequence + "'.";
      goal_handle->abort(result);
      return;
    }

    std::uint16_t shot_id = 0;
    {
      const std::lock_guard<std::mutex> lock{ mutex_ };
      fired_.push_back(sequence);
      shot_id = static_cast<std::uint16_t>(fired_.size());
    }
    std::this_thread::sleep_for(duration_);

    result->shot_id = shot_id;
    result->duration_us =
        static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration_).count());
    goal_handle->succeed(result);
  }

  rclcpp::Node::SharedPtr node_;
  const std::set<std::string> sequences_;
  const std::string default_sequence_;
  const std::chrono::milliseconds duration_;
  rclcpp_action::Server<Shoot>::SharedPtr server_;
  rclcpp::executors::SingleThreadedExecutor executor_;
  std::thread spinner_;

  mutable std::mutex mutex_;
  std::vector<std::string> fired_;
  std::vector<std::thread> shots_;
};

}  // namespace stepit_tests
