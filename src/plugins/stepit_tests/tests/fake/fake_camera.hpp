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
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <stepit_camera_msgs/msg/picture.hpp>

namespace stepit_tests
{

/**
 * @brief A stand-in for StepIt Camera: each time its shutter is released, it
 * reports a picture on /camera/picture, as the real driver does once it has
 * downloaded it. It can ignore the next releases, as the real camera
 * sometimes does.
 */
class FakeCamera
{
public:
  using Picture = stepit_camera_msgs::msg::Picture;

  FakeCamera() : node_{ std::make_shared<rclcpp::Node>("fake_camera") }
  {
    publisher_ = node_->create_publisher<Picture>("/camera/picture", rclcpp::QoS{ 10 });
  }

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
    picture.path = "/tmp/" + picture.name;
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

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<Picture>::SharedPtr publisher_;
  std::atomic<int> ignore_{ 0 };
  std::atomic<int> taken_{ 0 };
};

}  // namespace stepit_tests
