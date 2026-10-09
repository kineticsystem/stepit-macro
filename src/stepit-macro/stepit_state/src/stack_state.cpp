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

#include "stepit_state/stack_state.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace stepit_state
{
namespace
{
constexpr auto kStatePrefix = "state.";
constexpr auto kDefaultsPrefix = "defaults.";

rcl_interfaces::msg::ParameterDescriptor describe(const std::string& description, bool read_only)
{
  rcl_interfaces::msg::ParameterDescriptor descriptor;
  descriptor.description = description;
  descriptor.read_only = read_only;
  return descriptor;
}

bool contains(const std::vector<std::string>& names, const std::string& name)
{
  return std::find(names.begin(), names.end(), name) != names.end();
}

/// @brief A number of a parameter, whether the YAML wrote it as an integer or a double.
double asNumber(const std::string& name, const rclcpp::ParameterValue& value)
{
  switch (value.get_type())
  {
    case rclcpp::ParameterType::PARAMETER_DOUBLE:
      return value.get<double>();
    case rclcpp::ParameterType::PARAMETER_INTEGER:
      return static_cast<double>(value.get<std::int64_t>());
    default:
      throw std::invalid_argument("The parameter " + name + " must be a number");
  }
}
}  // namespace

std::filesystem::path expandHome(const std::string& path)
{
  if (path.rfind("~/", 0) == 0)
  {
    if (const char* home = std::getenv("HOME"))
    {
      return std::filesystem::path(home) / path.substr(2);
    }
  }
  return path;
}

Values readStateFile(const std::filesystem::path& path)
{
  const auto document = YAML::LoadFile(path.string());
  Values values;
  if (!document.IsDefined() || document.IsNull())
  {
    return values;
  }
  if (!document.IsMap())
  {
    throw std::runtime_error(path.string() + " is not a map of names to numbers");
  }
  for (const auto& entry : document)
  {
    const auto& value = entry.second;
    values[entry.first.as<std::string>()] =
        value.IsSequence() ? value.as<std::vector<double>>() : std::vector<double>{ value.as<double>() };
  }
  return values;
}

void writeStateFile(const std::filesystem::path& path, const Values& values)
{
  YAML::Node document(YAML::NodeType::Map);
  for (const auto& [name, numbers] : values)
  {
    YAML::Node list(YAML::NodeType::Sequence);
    list.SetStyle(YAML::EmitterStyle::Flow);
    for (const double number : numbers)
    {
      list.push_back(number);
    }
    document[name] = list;
  }

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

StackState::StackState(const rclcpp::NodeOptions& options) : rclcpp::Node("stack_state", options)
{
  file_ = expandHome(declare_parameter<std::string>("state_file", kDefaultStateFile,
                                                    describe("The YAML file of the saved values", true)));
  saved_ = declare_parameter<std::vector<std::string>>(
      "saved", { "shots", "angles", "turn" },
      describe("The values kept in state_file across restarts, e.g. the counts of a stack", true));
  const auto forgotten = declare_parameter<std::vector<std::string>>(
      "forgotten", { "near", "far" },
      describe("The values in memory only, empty at every start, e.g. the marks of the rail", true));
  for (const auto& name : forgotten)
  {
    if (contains(saved_, name))
    {
      throw std::invalid_argument(name + " cannot be both saved and forgotten");
    }
  }

  // The defaults of the saved values, by name: those the parameter file gives.
  std::map<std::string, double> defaults;
  for (const auto& [name, value] : get_node_parameters_interface()->get_parameter_overrides())
  {
    if (name.rfind(kDefaultsPrefix, 0) == 0)
    {
      defaults[name.substr(std::string(kDefaultsPrefix).size())] = asNumber(name, value);
      declare_parameter(name, value, describe("The value of a saved value while state_file has none", true));
    }
  }

  // The saved values, from the file, or their defaults; a file that cannot be
  // read is reported, and replaced by the next value saved.
  Values stored;
  try
  {
    if (std::filesystem::exists(file_))
    {
      stored = readStateFile(file_);
    }
  }
  catch (const std::exception& error)
  {
    RCLCPP_WARN(get_logger(), "Cannot read %s, starting from the defaults: %s", file_.c_str(), error.what());
  }
  for (const auto& name : saved_)
  {
    std::vector<double> value;
    if (const auto it = stored.find(name); it != stored.end())
    {
      value = it->second;
    }
    else if (const auto fallback = defaults.find(name); fallback != defaults.end())
    {
      value = { fallback->second };
    }
    declare_parameter(kStatePrefix + name, rclcpp::ParameterValue(value),
                      describe("Saved in state_file: survives a restart", false));
  }
  for (const auto& name : forgotten)
  {
    declare_parameter(kStatePrefix + name, rclcpp::ParameterValue(std::vector<double>{}),
                      describe("In memory only: empty at every start, which means not set", false));
  }

  callback_ = add_on_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter>& parameters) { return onSetParameters(parameters); });
  RCLCPP_INFO(get_logger(), "Saving %zu value(s) in %s, forgetting %zu at every start", saved_.size(), file_.c_str(),
              forgotten.size());
}

rcl_interfaces::msg::SetParametersResult StackState::onSetParameters(const std::vector<rclcpp::Parameter>& parameters)
{
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;

  Values changed;
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
    const auto key = name.substr(std::string(kStatePrefix).size());
    if (contains(saved_, key))
    {
      changed[key] = parameter.as_double_array();
    }
  }
  if (changed.empty())
  {
    return result;
  }

  // The whole file: every saved value as it is, with the changes.
  Values values;
  for (const auto& name : saved_)
  {
    const auto it = changed.find(name);
    values[name] = it != changed.end() ? it->second : get_parameter(kStatePrefix + name).as_double_array();
  }
  try
  {
    writeStateFile(file_, values);
  }
  catch (const std::exception& error)
  {
    result.successful = false;
    result.reason = std::string("cannot save in ") + file_.string() + ": " + error.what();
    RCLCPP_ERROR(get_logger(), "%s", result.reason.c_str());
  }
  return result;
}

}  // namespace stepit_state
