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
constexpr auto kPicturesFolder = "pictures_folder";

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

/// @brief The value of a parameter: declared, or else given by the node's parameter file, if either.
std::optional<rclcpp::ParameterValue> valueOf(rclcpp::Node& node, const std::string& name)
{
  if (node.has_parameter(name))
  {
    return node.get_parameter(name).get_parameter_value();
  }
  const auto& overrides = node.get_node_parameters_interface()->get_parameter_overrides();
  if (const auto it = overrides.find(name); it != overrides.end())
  {
    return it->second;
  }
  return std::nullopt;
}

std::optional<double> optionalNumber(rclcpp::Node& node, const std::string& name)
{
  const auto value = valueOf(node, name);
  if (!value)
  {
    return std::nullopt;
  }
  return asNumber(*value);
}

/// @brief A path with a leading `~/` replaced by the home folder.
std::string expandHome(std::string path)
{
  if (path.rfind("~/", 0) == 0)
  {
    if (const char* home = std::getenv("HOME"))
    {
      path = std::string(home) + path.substr(1);
    }
  }
  return path;
}
}  // namespace

double overshootParameter(rclcpp::Node& node, const std::string& joint)
{
  return optionalNumber(node, kOvershootPrefix + joint).value_or(0.0);
}

std::optional<double> mmPerTurnParameter(rclcpp::Node& node, const std::string& joint)
{
  return optionalNumber(node, kMmPerTurnPrefix + joint);
}

std::optional<double> degPerTurnParameter(rclcpp::Node& node, const std::string& joint)
{
  return optionalNumber(node, kDegPerTurnPrefix + joint);
}

std::string picturesFolderParameter(rclcpp::Node& node)
{
  const auto value = valueOf(node, kPicturesFolder);
  return expandHome(value && value->get_type() == rclcpp::ParameterType::PARAMETER_STRING ? value->get<std::string>() :
                                                                                            kDefaultPicturesFolder);
}

}  // namespace stepit_behaviors
