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
#include <utility>
#include <vector>

#include "stepit_behaviors/parameters.hpp"
#include "stepit_behaviors/picture_folder.hpp"
#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{
namespace
{

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

/// @brief Names as a JSON list, one per line, indented as a value of the top object.
std::string jsonList(const std::vector<std::string>& names)
{
  std::string out = "[";
  for (std::size_t i = 0; i < names.size(); ++i)
  {
    out += (i == 0 ? "\n    " : ",\n    ") + jsonString(names[i]);
  }
  return out + (names.empty() ? "]" : "\n  ]");
}

/// @brief Writes `text` to a temporary file next to `path`, then renames it: the file is either whole or absent.
void writeWhole(const std::filesystem::path& path, const std::string& text)
{
  const auto temporary = path.parent_path() / ("." + path.filename().string() + ".tmp");
  {
    std::ofstream out(temporary);
    out << text;
    if (!out)
    {
      throw std::runtime_error("cannot write " + temporary.string());
    }
  }
  std::filesystem::rename(temporary, path);
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
  json << ",\n  \"finished\": " << jsonString(now()) << ",\n  \"files\": " << jsonList(files) << "\n}\n";
  writeWhole(folder / kStackDoneFile, json.str());
}

/// @brief Writes all_stacks.json into `folder`, the stack's folder, with its angles' folders. Throws on failure.
void writeAllStacksFile(const std::filesystem::path& folder, const std::string& relative, int shots, int angles)
{
  if (!std::filesystem::is_directory(folder))
  {
    throw std::runtime_error(folder.string() + " is not a folder");
  }
  std::vector<std::string> stacks;
  for (const auto& entry : std::filesystem::directory_iterator(folder))
  {
    const auto name = entry.path().filename().string();
    if (entry.is_directory() && name.rfind('.', 0) != 0)
    {
      stacks.push_back(name);
    }
  }
  std::sort(stacks.begin(), stacks.end());

  std::ostringstream json;
  json << "{\n  \"folder\": " << jsonString(relative) << ",\n  \"shots\": " << shots << ",\n  \"angles\": " << angles
       << ",\n  \"finished\": " << jsonString(now()) << ",\n  \"stacks\": " << jsonList(stacks) << "\n}\n";
  writeWhole(folder / kAllStacksDoneFile, json.str());
}
/// @brief `publisher` if it publishes on `topic`, or a new one on it.
LatchedPublisher onTopic(LatchedPublisher publisher, rclcpp::Node& node, const std::string& topic)
{
  return publisher && publisher->get_topic_name() == topic ? publisher : latchedPublisher(node, topic);
}
}  // namespace

LatchedPublisher latchedPublisher(rclcpp::Node& node, const std::string& topic)
{
  return node.create_publisher<std_msgs::msg::String>(topic, rclcpp::QoS{ 1 }.reliable().transient_local());
}

StackDone::StackDone(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params,
                     LatchedPublisher publisher)
  : BT::SyncActionNode(name, config), node_(params.nh), publisher_(std::move(publisher))
{
}

BT::PortsList StackDone::providedPorts()
{
  return {
    BT::InputPort<std::string>("topic_name", kStackDoneTopic, "the latched topic of the finished stacks"),
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

  publisher_ = onTopic(publisher_, *node, getInput<std::string>("topic_name").value_or(kStackDoneTopic));
  std_msgs::msg::String message;
  message.data = relative;
  publisher_->publish(message);
  RCLCPP_INFO(node->get_logger(), "%s: the stack %s is done", name().c_str(), relative.c_str());
  return BT::NodeStatus::SUCCESS;
}

AllStacksDone::AllStacksDone(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params,
                             LatchedPublisher publisher)
  : BT::SyncActionNode(name, config), node_(params.nh), publisher_(std::move(publisher))
{
}

BT::PortsList AllStacksDone::providedPorts()
{
  return {
    BT::InputPort<std::string>("topic_name", kAllStacksDoneTopic, "the latched topic of the finished focus stacks"),
    BT::InputPort<std::string>("folder", "the stack's folder under the pictures folder, as given to SetPictureFolder"),
    BT::InputPort<BT::AnyTypeAllowed>("shots", "how many shots each stack has, e.g. 10"),
    BT::InputPort<BT::AnyTypeAllowed>("angles", "how many angles, one stack each, e.g. 35"),
  };
}

BT::NodeStatus AllStacksDone::tick()
{
  const auto node = node_.lock();
  if (!node)
  {
    throw BT::RuntimeError("AllStacksDone: the ROS node went out of scope");
  }
  const auto folder = getInput<std::string>("folder");
  if (!folder || folder->empty())
  {
    throw BT::RuntimeError("AllStacksDone: [folder] is required");
  }
  const int shots = static_cast<int>(std::lround(requireNumbers(*this, "shots").front()));
  const int angles = static_cast<int>(std::lround(requireNumbers(*this, "angles").front()));

  try
  {
    writeAllStacksFile(std::filesystem::path(picturesFolderParameter(*node)) / *folder, *folder, shots, angles);
  }
  catch (const std::exception& error)
  {
    RCLCPP_ERROR(node->get_logger(), "%s: cannot write %s of %s: %s", name().c_str(), kAllStacksDoneFile,
                 folder->c_str(), error.what());
  }

  publisher_ = onTopic(publisher_, *node, getInput<std::string>("topic_name").value_or(kAllStacksDoneTopic));
  std_msgs::msg::String message;
  message.data = *folder;
  publisher_->publish(message);
  RCLCPP_INFO(node->get_logger(), "%s: every stack of %s is done", name().c_str(), folder->c_str());
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
