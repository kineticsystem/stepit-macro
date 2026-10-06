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

#include "stepit_behaviors/expect_picture.hpp"

namespace stepit_behaviors
{

ExpectPicture::ExpectPicture(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::DecoratorNode(name, config), node_(params.nh), logger_(rclcpp::get_logger("ExpectPicture"))
{
  if (const auto node = node_.lock())
  {
    logger_ = node->get_logger();
  }
}

BT::PortsList ExpectPicture::providedPorts()
{
  return {
    BT::InputPort<std::string>("topic_name", "/camera/picture", "the topic StepIt Camera reports its pictures on"),
    BT::InputPort<double>("timeout", 15.0, "how long the picture may take after the shot, in seconds"),
    BT::InputPort<int>("files", 1, "how many files a shot gives: 2 for RAW+JPEG"),
  };
}

void ExpectPicture::connect(const std::string& topic)
{
  if (subscription_)
  {
    return;
  }
  auto node = node_.lock();
  if (!node)
  {
    throw BT::RuntimeError("ExpectPicture: the ROS node went out of scope");
  }
  // A callback group of our own, spun only when ticked, as the nodes of
  // BehaviorTree.ROS2 do: the tree is ticked from a single thread.
  callback_group_ = node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
  executor_.add_callback_group(callback_group_, node->get_node_base_interface());
  rclcpp::SubscriptionOptions options;
  options.callback_group = callback_group_;
  subscription_ = node->create_subscription<Picture>(
      topic, rclcpp::QoS{ 10 },
      [this](const Picture::SharedPtr msg) {
        if (listening_)
        {
          ++pictures_;
          RCLCPP_INFO(logger_, "%s: picture %s", name().c_str(), msg->name.c_str());
        }
      },
      options);
}

BT::NodeStatus ExpectPicture::tick()
{
  if (status() == BT::NodeStatus::IDLE)
  {
    const auto topic = getInput<std::string>("topic_name");
    const auto files = getInput<int>("files");
    if (!topic || !files || files.value() < 1)
    {
      throw BT::RuntimeError("ExpectPicture: [topic_name] must be a topic, [files] at least 1");
    }
    connect(topic.value());
    // Drop what came before: a picture of an earlier shot is not this one's.
    listening_ = false;
    executor_.spin_some();
    listening_ = true;
    files_ = files.value();
    pictures_ = 0;
    shot_ = false;
  }
  setStatus(BT::NodeStatus::RUNNING);

  if (!shot_)
  {
    switch (child_node_->executeTick())
    {
      case BT::NodeStatus::RUNNING:
        return BT::NodeStatus::RUNNING;
      case BT::NodeStatus::SUCCESS:
      {
        resetChild();
        shot_ = true;
        const auto timeout = getInput<double>("timeout").value_or(15.0);
        deadline_ = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                                           std::chrono::duration<double>(timeout));
        break;
      }
      case BT::NodeStatus::FAILURE:
        resetChild();
        listening_ = false;
        return BT::NodeStatus::FAILURE;
      default:
        throw BT::LogicError("ExpectPicture: the child returned ", BT::toStr(child_node_->status()));
    }
  }

  executor_.spin_some();
  if (pictures_ >= files_)
  {
    listening_ = false;
    return BT::NodeStatus::SUCCESS;
  }
  if (std::chrono::steady_clock::now() >= deadline_)
  {
    listening_ = false;
    RCLCPP_ERROR(logger_, "%s: the shot was fired, but no picture came on %s: the camera ignored it?", name().c_str(),
                 subscription_->get_topic_name());
    return BT::NodeStatus::FAILURE;
  }
  return BT::NodeStatus::RUNNING;
}

void ExpectPicture::halt()
{
  listening_ = false;
  shot_ = false;
  BT::DecoratorNode::halt();
}

}  // namespace stepit_behaviors
