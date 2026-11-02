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

#include "stepit_behaviors/pause_live_view.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <string>

namespace stepit_behaviors
{

PauseLiveView::PauseLiveView(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::DecoratorNode(name, config), logger_(rclcpp::get_logger("PauseLiveView"))
{
  const auto node = params.nh.lock();
  if (!node)
  {
    throw BT::RuntimeError("PauseLiveView: the ROS node went out of scope");
  }
  logger_ = node->get_logger();
  const auto camera = getInput<std::string>("camera").value_or("/camera");

  // A callback group of its own, spun only when ticked, as the nodes of
  // BehaviorTree.ROS2 do. Made with the tree, so that the latched status and
  // the services are found by the time the node runs.
  callback_group_ = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
  executor_.add_callback_group(callback_group_, node->get_node_base_interface());
  rclcpp::SubscriptionOptions options;
  options.callback_group = callback_group_;
  status_subscription_ = node->create_subscription<CameraStatus>(
      camera + "/status", rclcpp::QoS{ 1 }.reliable().transient_local(),
      [this](const CameraStatus::SharedPtr status) { streaming_ = status->streaming; }, options);
  stop_client_ = node->create_client<Trigger>(camera + "/stop_streaming", rclcpp::ServicesQoS(), callback_group_);
  start_client_ = node->create_client<Trigger>(camera + "/start_streaming", rclcpp::ServicesQoS(), callback_group_);
}

BT::PortsList PauseLiveView::providedPorts()
{
  return {
    BT::InputPort<std::string>("camera", "/camera", "the node of StepIt Camera"),
    BT::InputPort<bool>("resume", true, "switch the live view on again after the child, if it was on"),
    BT::InputPort<double>("timeout", 10.0,
                          "how long to wait for the camera's status, then for it to leave the live view, in seconds"),
  };
}

BT::NodeStatus PauseLiveView::tick()
{
  const auto timeout = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(getInput<double>("timeout").value_or(10.0)));
  if (status() == BT::NodeStatus::IDLE)
  {
    paused_ = false;
    stopping_.reset();
    phase_ = Phase::WaitingForStatus;
    deadline_ = std::chrono::steady_clock::now() + timeout;
  }
  setStatus(BT::NodeStatus::RUNNING);
  executor_.spin_some();

  if (phase_ == Phase::WaitingForStatus)
  {
    if (!streaming_.has_value() && std::chrono::steady_clock::now() < deadline_)
    {
      return BT::NodeStatus::RUNNING;
    }
    if (!streaming_.value_or(false))
    {
      phase_ = Phase::Running;
    }
    else
    {
      if (!stop_client_->service_is_ready())
      {
        RCLCPP_ERROR(logger_, "%s: the live view is on, and %s is not there to stop it", name().c_str(),
                     stop_client_->get_service_name());
        return BT::NodeStatus::FAILURE;
      }
      stop_client_->prune_pending_requests();
      stopping_ = stop_client_->async_send_request(std::make_shared<Trigger::Request>());
      phase_ = Phase::Stopping;
      deadline_ = std::chrono::steady_clock::now() + timeout;
      RCLCPP_INFO(logger_, "%s: the live view is on: switching it off", name().c_str());
    }
  }

  if (phase_ == Phase::Stopping)
  {
    if (stopping_->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
      if (std::chrono::steady_clock::now() < deadline_)
      {
        return BT::NodeStatus::RUNNING;
      }
      RCLCPP_ERROR(logger_, "%s: the camera did not leave the live view in time", name().c_str());
      return BT::NodeStatus::FAILURE;
    }
    const auto response = stopping_->future.get();
    stopping_.reset();
    if (!response->success)
    {
      RCLCPP_ERROR(logger_, "%s: the live view could not be stopped: %s", name().c_str(), response->message.c_str());
      return BT::NodeStatus::FAILURE;
    }
    paused_ = true;
    phase_ = Phase::Running;
  }

  const auto status = child_node_->executeTick();
  if (status == BT::NodeStatus::SUCCESS || status == BT::NodeStatus::FAILURE)
  {
    resetChild();
    resume();
  }
  return status;
}

void PauseLiveView::halt()
{
  BT::DecoratorNode::halt();
  stopping_.reset();
  resume();
}

void PauseLiveView::resume()
{
  if (!paused_)
  {
    return;
  }
  paused_ = false;
  if (!getInput<bool>("resume").value_or(true))
  {
    return;
  }
  if (!start_client_->service_is_ready())
  {
    RCLCPP_WARN(logger_, "%s: %s is not there, the live view stays off", name().c_str(),
                start_client_->get_service_name());
    return;
  }
  // Nobody reads the answers: forget the requests sent before this one.
  start_client_->prune_pending_requests();
  start_client_->async_send_request(std::make_shared<Trigger::Request>());
  RCLCPP_INFO(logger_, "%s: switching the live view on again", name().c_str());
}

}  // namespace stepit_behaviors
