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
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <stepit_camera_msgs/msg/picture.hpp>

namespace stepit_tests
{

/**
 * @brief A stand-in for StepIt Camera: each time its shutter is released, it
 * reports a picture on /camera/picture, as the real driver does once it has
 * downloaded it, in the folder its parameter `folder` names, which a node sets
 * through /camera/set_parameters. It records every folder it is given, and can
 * ignore the next releases, as the real camera sometimes does.
 */
class FakeCamera
{
public:
  using Picture = stepit_camera_msgs::msg::Picture;

  FakeCamera() : node_{ std::make_shared<rclcpp::Node>("camera") }
  {
    publisher_ = node_->create_publisher<Picture>("/camera/picture", rclcpp::QoS{ 10 });
    node_->declare_parameter("folder", "");
    on_set_ = node_->add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter>& parameters) {
      rcl_interfaces::msg::SetParametersResult result;
      result.successful = true;
      for (const auto& parameter : parameters)
      {
        if (parameter.get_name() == "folder")
        {
          const std::lock_guard<std::mutex> lock{ mutex_ };
          folders_.push_back(parameter.as_string());
        }
      }
      return result;
    });
    executor_.add_node(node_);
    spinner_ = std::thread{ [this]() { executor_.spin(); } };
  }

  ~FakeCamera()
  {
    executor_.cancel();
    if (spinner_.joinable())
    {
      spinner_.join();
    }
    executor_.remove_node(node_);
  }

  FakeCamera(const FakeCamera&) = delete;
  FakeCamera& operator=(const FakeCamera&) = delete;

  /// @brief The shutter is released: a picture, unless the camera ignores it.
  void release()
  {
    if (ignore_ > 0)
    {
      --ignore_;
      return;
    }
    Picture picture;
    picture.header.stamp = node_->now();
    picture.name = "IMG_" + std::to_string(++taken_) + ".CR2";
    const auto folder = node_->get_parameter("folder").as_string();
    picture.relative_path = folder.empty() ? picture.name : folder + "/" + picture.name;
    picture.path = "/tmp/" + picture.relative_path;
    {
      const std::lock_guard<std::mutex> lock{ mutex_ };
      pictures_.push_back(picture.relative_path);
    }
    publisher_->publish(picture);
  }

  /// @brief Ignore the next `count` releases, taking no picture.
  void ignore(int count)
  {
    ignore_ = count;
  }

  /// @brief How many pictures it took.
  int taken() const
  {
    return taken_;
  }

  /// @brief Every folder it was given, in order.
  std::vector<std::string> folders() const
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return folders_;
  }

  /// @brief Where each picture went, under the pictures folder, in order.
  std::vector<std::string> pictures() const
  {
    const std::lock_guard<std::mutex> lock{ mutex_ };
    return pictures_;
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<Picture>::SharedPtr publisher_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr on_set_;
  rclcpp::executors::SingleThreadedExecutor executor_;
  std::thread spinner_;
  std::atomic<int> ignore_{ 0 };
  std::atomic<int> taken_{ 0 };

  mutable std::mutex mutex_;
  std::vector<std::string> folders_;
  std::vector<std::string> pictures_;
};

}  // namespace stepit_tests
