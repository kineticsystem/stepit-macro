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

#include "stepit_behaviors/values_file.hpp"

#include <filesystem>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "stepit_behaviors/parameters.hpp"
#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{
namespace
{

BT::PortsList::value_type filePort()
{
  auto port = BT::InputPort<std::string>("file", "the YAML file; by default the commander's parameter state_file");
  port.second.setDefaultValue(std::string());
  return port;
}

/// @brief The file of the node's port `file`, or the commander's `state_file`.
std::filesystem::path fileOf(const BT::TreeNode& tree_node, const std::weak_ptr<rclcpp::Node>& weak)
{
  const auto file = tree_node.getInput<std::string>("file");
  if (file && !file.value().empty())
  {
    return file.value();
  }
  const auto node = weak.lock();
  return node ? stateFileParameter(*node) : std::string(kDefaultStateFile);
}

rclcpp::Logger loggerOf(const std::weak_ptr<rclcpp::Node>& weak)
{
  const auto node = weak.lock();
  return node ? node->get_logger() : rclcpp::get_logger("stepit_behaviors");
}

std::string requireKey(const BT::TreeNode& node)
{
  const auto key = node.getInput<std::string>("key");
  if (!key || key.value().empty())
  {
    throw BT::RuntimeError(node.registrationName(), ": [key] must name the values");
  }
  return key.value();
}

}  // namespace

SaveValues::SaveValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList SaveValues::providedPorts()
{
  return {
    BT::InputPort<std::string>("key", "the name to save the values under, e.g. near"),
    BT::InputPort<BT::AnyTypeAllowed>("values", "the numbers to save: a number, or a list"),
    filePort(),
  };
}

BT::NodeStatus SaveValues::tick()
{
  const auto key = requireKey(*this);
  const auto values = requireNumbers(*this, "values");
  const auto path = fileOf(*this, node_);
  const auto logger = loggerOf(node_);

  try
  {
    writeStateValues(path, key, values);
  }
  catch (const std::exception& error)
  {
    RCLCPP_ERROR(logger, "%s: cannot save %s in %s: %s", name().c_str(), key.c_str(), path.c_str(), error.what());
    return BT::NodeStatus::FAILURE;
  }

  RCLCPP_INFO(logger, "%s: saved %s in %s", name().c_str(), key.c_str(), path.c_str());
  if (const auto node = node_.lock())
  {
    publishState(*node, key, values);
  }
  return BT::NodeStatus::SUCCESS;
}

LoadValues::LoadValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList LoadValues::providedPorts()
{
  return {
    BT::InputPort<std::string>("key", "the name the values were saved under, e.g. near"),
    BT::OutputPort<std::vector<double>>("values", "the numbers saved, always a list"),
    filePort(),
  };
}

BT::NodeStatus LoadValues::tick()
{
  const auto key = requireKey(*this);
  const auto path = fileOf(*this, node_);
  const auto logger = loggerOf(node_);

  std::vector<double> values;
  try
  {
    if (!std::filesystem::exists(path))
    {
      RCLCPP_ERROR(logger, "%s: nothing saved yet: %s is missing", name().c_str(), path.c_str());
      return BT::NodeStatus::FAILURE;
    }
    const auto entry = YAML::LoadFile(path.string())[key];
    if (!entry)
    {
      RCLCPP_ERROR(logger, "%s: no %s saved in %s", name().c_str(), key.c_str(), path.c_str());
      return BT::NodeStatus::FAILURE;
    }
    if (entry.IsSequence())
    {
      values = entry.as<std::vector<double>>();
    }
    else
    {
      values = { entry.as<double>() };
    }
  }
  catch (const std::exception& error)
  {
    RCLCPP_ERROR(logger, "%s: cannot read %s from %s: %s", name().c_str(), key.c_str(), path.c_str(), error.what());
    return BT::NodeStatus::FAILURE;
  }

  if (values.empty())
  {
    RCLCPP_ERROR(logger, "%s: %s in %s holds no value", name().c_str(), key.c_str(), path.c_str());
    return BT::NodeStatus::FAILURE;
  }
  setOutput("values", values);
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
