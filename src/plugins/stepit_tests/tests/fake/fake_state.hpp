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

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>

namespace stepit_tests
{

/**
 * @brief A stand-in for stack_state, the node that keeps the state of the
 * rig: each value is its parameter `state.<name>`, a list of numbers, which
 * clients read and set through its parameter services, as the real one does.
 * A name it was not given does not exist, and an empty list means not set.
 * It can refuse every change, as the real node does when it cannot save one.
 */
class FakeState
{
public:
  explicit FakeState(const std::map<std::string, std::vector<double>>& values = { { "near", {} }, { "far", {} } })
    : node_{ std::make_shared<rclcpp::Node>("stack_state") }
  {
    for (const auto& [name, value] : values)
    {
      node_->declare_parameter("state." + name, rclcpp::ParameterValue(value));
    }
    callback_ = node_->add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter>&) {
      rcl_interfaces::msg::SetParametersResult result;
      result.successful = !refuse_;
      result.reason = refuse_ ? "cannot save" : "";
      return result;
    });
    executor_.add_node(node_);
    spinner_ = std::thread{ [this]() { executor_.spin(); } };
  }

  ~FakeState()
  {
    executor_.cancel();
    if (spinner_.joinable())
    {
      spinner_.join();
    }
    executor_.remove_node(node_);
  }

  FakeState(const FakeState&) = delete;
  FakeState& operator=(const FakeState&) = delete;

  /// @brief The values saved under a name, empty if none.
  std::vector<double> get(const std::string& name) const
  {
    return node_->get_parameter("state." + name).as_double_array();
  }

  /// @brief Save values under a name, as a page would.
  void set(const std::string& name, const std::vector<double>& values)
  {
    node_->set_parameter(rclcpp::Parameter("state." + name, values));
  }

  /// @brief Refuse every change from now on, or accept them again.
  void refuse(bool refuse)
  {
    refuse_ = refuse;
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr callback_;
  std::atomic<bool> refuse_{ false };
  rclcpp::executors::SingleThreadedExecutor executor_;
  std::thread spinner_;
};

}  // namespace stepit_tests
