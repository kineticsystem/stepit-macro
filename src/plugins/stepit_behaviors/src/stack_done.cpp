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

#include "stepit_behaviors/stack_done.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "stepit_behaviors/parameters.hpp"
#include "stepit_behaviors/picture_folder.hpp"
#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{
namespace
{
constexpr auto kDefaultTopic = "/focus_stack/stack_done";

/// @brief A text as a JSON string, quotes included.
std::string jsonString(const std::string& text)
{
  std::string out = "\"";
  for (const char c : text)
  {
    if (c == '"' || c == '\\')
    {
      out += '\\';
      out += c;
    }
    else if (static_cast<unsigned char>(c) < 0x20)
    {
      char escaped[8];
      std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(c));
      out += escaped;
    }
    else
    {
      out += c;
    }
  }
  return out + "\"";
}

/// @brief The local time, as ISO 8601, e.g. 2026-10-06T15:24:31.
std::string now()
{
  const std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_r(&time, &local);
  char text[32];
  std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%S", &local);
  return text;
}

/// @brief Writes stack.json into `folder`, which holds the pictures of the stack. Throws on failure.
void writeStackFile(const std::filesystem::path& folder, const std::string& relative, int shots, int index,
                    std::optional<double> degrees)
{
  if (!std::filesystem::is_directory(folder))
  {
    throw std::runtime_error(folder.string() + " is not a folder");
  }
  std::vector<std::string> files;
  for (const auto& entry : std::filesystem::directory_iterator(folder))
  {
    const auto file = entry.path().filename().string();
    if (entry.is_regular_file() && file != kStackDoneFile && file.rfind('.', 0) != 0)
    {
      files.push_back(file);
    }
  }
  std::sort(files.begin(), files.end());

  std::ostringstream json;
  json << "{\n  \"folder\": " << jsonString(relative) << ",\n  \"shots\": " << shots << ",\n  \"angle\": " << index + 1;
  if (degrees)
  {
    json << ",\n  \"degrees\": " << *degrees;
  }
  json << ",\n  \"finished\": " << jsonString(now()) << ",\n  \"files\": [";
  for (std::size_t i = 0; i < files.size(); ++i)
  {
    json << (i == 0 ? "\n    " : ",\n    ") << jsonString(files[i]);
  }
  json << (files.empty() ? "]\n}\n" : "\n  ]\n}\n");

  // A temporary file first, renamed: stack.json is either whole or absent.
  const auto path = folder / kStackDoneFile;
  auto temporary = folder / (std::string(".") + kStackDoneFile + ".tmp");
  {
    std::ofstream out(temporary);
    out << json.str();
    if (!out)
    {
      throw std::runtime_error("cannot write " + temporary.string());
    }
  }
  std::filesystem::rename(temporary, path);
}
}  // namespace

StackDone::StackDone(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList StackDone::providedPorts()
{
  return {
    BT::InputPort<std::string>("topic_name", kDefaultTopic, "the latched topic of the finished stacks"),
    BT::InputPort<std::string>("folder", "the stack's folder under the pictures folder, as given to SetPictureFolder"),
    BT::InputPort<int>("index", "the angle of the stack, from 0, as given to SetPictureFolder"),
    optionalInput("degrees", "the angle in degrees, as given to SetPictureFolder"),
    BT::InputPort<BT::AnyTypeAllowed>("shots", "how many shots the stack has, e.g. 10"),
  };
}

BT::NodeStatus StackDone::tick()
{
  const auto node = node_.lock();
  if (!node)
  {
    throw BT::RuntimeError("StackDone: the ROS node went out of scope");
  }
  const auto folder = getInput<std::string>("folder");
  const auto index = getInput<int>("index");
  if (!folder || !index)
  {
    throw BT::RuntimeError("StackDone: [folder] and [index] are required");
  }
  std::optional<double> degrees;
  if (isGiven(*this, "degrees"))
  {
    degrees = requireNumbers(*this, "degrees").front();
  }
  const int shots = static_cast<int>(std::lround(requireNumbers(*this, "shots").front()));
  const auto relative = (folder->empty() ? "" : *folder + "/") + angleFolder(*index, degrees);

  try
  {
    writeStackFile(std::filesystem::path(picturesFolderParameter(*node)) / relative, relative, shots, *index, degrees);
  }
  catch (const std::exception& error)
  {
    RCLCPP_ERROR(node->get_logger(), "%s: cannot write %s of %s: %s", name().c_str(), kStackDoneFile, relative.c_str(),
                 error.what());
  }

  if (!publisher_)
  {
    const auto topic = getInput<std::string>("topic_name").value_or(kDefaultTopic);
    // Latched: the last stack reaches a subscriber that comes later.
    publisher_ = node->create_publisher<std_msgs::msg::String>(topic, rclcpp::QoS{ 1 }.reliable().transient_local());
  }
  std_msgs::msg::String message;
  message.data = relative;
  publisher_->publish(message);
  RCLCPP_INFO(node->get_logger(), "%s: the stack %s is done", name().c_str(), relative.c_str());
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
