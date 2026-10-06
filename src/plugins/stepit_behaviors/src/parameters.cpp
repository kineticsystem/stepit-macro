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
