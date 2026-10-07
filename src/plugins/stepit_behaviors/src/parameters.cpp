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

#include "stepit_behaviors/parameters.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace stepit_behaviors
{
namespace
{
constexpr auto kOvershootPrefix = "overshoot.";
constexpr auto kMmPerTurnPrefix = "mm_per_turn.";
constexpr auto kDegPerTurnPrefix = "deg_per_turn.";
/// What the rig configures for its pages, e.g. StepIt UI, which read it from the commander.
constexpr auto kFocusStackPrefix = "focus_stack.";
constexpr auto kStateFile = "state_file";
constexpr auto kStatePrefix = "state.";

/// @brief A number of a parameter, whether the YAML wrote it as an integer or a double.
double asNumber(const rclcpp::ParameterValue& value)
{
  switch (value.get_type())
  {
    case rclcpp::ParameterType::PARAMETER_DOUBLE:
      return value.get<double>();
    case rclcpp::ParameterType::PARAMETER_INTEGER:
      return static_cast<double>(value.get<std::int64_t>());
    default:
      return 0.0;
  }
}
}  // namespace

void declareParameters(rclcpp::Node& node)
{
  // The joints are the robot's, unknown here: one parameter for each the
  // parameter file lists. Declared with the type it gives, so that 1 and 1.0
  // are both accepted.
  for (const auto& [name, value] : node.get_node_parameters_interface()->get_parameter_overrides())
  {
    const bool ours = name.rfind(kOvershootPrefix, 0) == 0 || name.rfind(kMmPerTurnPrefix, 0) == 0 ||
                      name.rfind(kDegPerTurnPrefix, 0) == 0 || name.rfind(kFocusStackPrefix, 0) == 0;
    if (ours && !node.has_parameter(name))
    {
      node.declare_parameter(name, value);
    }
  }
  if (!node.has_parameter(kStateFile))
  {
    node.declare_parameter<std::string>(kStateFile, kDefaultStateFile);
  }

  // What the state file holds from before, e.g. the marks of a stack: the
  // pages read it as parameters.
  const std::filesystem::path path = stateFileParameter(node);
  try
  {
    if (std::filesystem::exists(path))
    {
      const auto document = YAML::LoadFile(path.string());
      for (const auto& entry : document)
      {
        const auto& value = entry.second;
        publishState(node, entry.first.as<std::string>(),
                     value.IsSequence() ? value.as<std::vector<double>>() : std::vector<double>{ value.as<double>() });
      }
    }
  }
  catch (const std::exception& error)
  {
    RCLCPP_WARN(node.get_logger(), "Cannot read the state file %s: %s", path.c_str(), error.what());
  }

  // What a page sets and the stack needs: the stage's turn either way and the
  // counts, from rig.yaml's defaults until a page sets them.
  for (const auto* key : { "turn", "shots", "angles" })
  {
    const auto config = std::string(kFocusStackPrefix) + key;
    if (!node.has_parameter(kStatePrefix + std::string(key)) && node.has_parameter(config))
    {
      publishState(node, key, { asNumber(node.get_parameter(config).get_parameter_value()) });
    }
  }

  // A page sets state.<key>: into the file, so that it survives a restart.
  // The callbacks of the node are kept as long as the process lives.
  static std::mutex mutex;
  static std::vector<rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr> callbacks;
  const std::lock_guard<std::mutex> lock{ mutex };
  callbacks.push_back(node.add_on_set_parameters_callback([&node](const std::vector<rclcpp::Parameter>& parameters) {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    for (const auto& parameter : parameters)
    {
      const auto& name = parameter.get_name();
      if (name.rfind(kStatePrefix, 0) != 0)
      {
        continue;
      }
      if (parameter.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE_ARRAY)
      {
        result.successful = false;
        result.reason = name + " must be a list of numbers";
        return result;
      }
      try
      {
        writeStateValues(stateFileParameter(node), name.substr(std::string(kStatePrefix).size()),
                         parameter.as_double_array());
      }
      catch (const std::exception& error)
      {
        result.successful = false;
        result.reason = std::string("cannot save ") + name + ": " + error.what();
        return result;
      }
    }
    return result;
  }));
}

void writeStateValues(const std::filesystem::path& path, const std::string& key, const std::vector<double>& values)
{
  YAML::Node document = std::filesystem::exists(path) ? YAML::LoadFile(path.string()) : YAML::Node();
  if (!document.IsMap())
  {
    document = YAML::Node(YAML::NodeType::Map);
  }
  YAML::Node list(YAML::NodeType::Sequence);
  list.SetStyle(YAML::EmitterStyle::Flow);
  for (const double value : values)
  {
    list.push_back(value);
  }
  document[key] = list;

  if (path.has_parent_path())
  {
    std::filesystem::create_directories(path.parent_path());
  }
  auto temporary = path;
  temporary += ".tmp";
  {
    std::ofstream out(temporary);
    out << document << '\n';
    if (!out)
    {
      throw std::runtime_error("cannot write " + temporary.string());
    }
  }
  std::filesystem::rename(temporary, path);
}

void publishState(rclcpp::Node& node, const std::string& key, const std::vector<double>& values)
{
  const auto name = kStatePrefix + key;
  if (!node.has_parameter(name))
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description =
        "What the objectives saved in the state file, e.g. the marks of a stack: set by SaveValues";
    node.declare_parameter(name, rclcpp::ParameterValue(values), descriptor);
    return;
  }
  node.set_parameter(rclcpp::Parameter(name, values));
}

double overshootParameter(rclcpp::Node& node, const std::string& joint)
{
  const auto name = kOvershootPrefix + joint;
  return node.has_parameter(name) ? asNumber(node.get_parameter(name).get_parameter_value()) : 0.0;
}

namespace
{
std::optional<double> optionalNumber(rclcpp::Node& node, const std::string& name)
{
  if (!node.has_parameter(name))
  {
    return std::nullopt;
  }
  return asNumber(node.get_parameter(name).get_parameter_value());
}
}  // namespace

std::optional<double> mmPerTurnParameter(rclcpp::Node& node, const std::string& joint)
{
  return optionalNumber(node, kMmPerTurnPrefix + joint);
}

std::optional<double> degPerTurnParameter(rclcpp::Node& node, const std::string& joint)
{
  return optionalNumber(node, kDegPerTurnPrefix + joint);
}

std::string stateFileParameter(rclcpp::Node& node)
{
  auto path = node.has_parameter(kStateFile) ? node.get_parameter(kStateFile).as_string() : kDefaultStateFile;
  if (path.rfind("~/", 0) == 0)
  {
    if (const char* home = std::getenv("HOME"))
    {
      path = std::string(home) + path.substr(1);
    }
  }
  return path;
}

}  // namespace stepit_behaviors
