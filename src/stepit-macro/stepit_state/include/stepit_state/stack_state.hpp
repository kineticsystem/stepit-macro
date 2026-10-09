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

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

namespace stepit_state
{

/// @brief Values by name, each a list of numbers, e.g. {"shots": [10]}.
using Values = std::map<std::string, std::vector<double>>;

/// @brief The default of `state_file`.
inline constexpr auto kDefaultStateFile = "~/.ros/stepit_state.yaml";

/// @brief A path with a leading `~/` replaced by the home folder.
std::filesystem::path expandHome(const std::string& path);

/**
 * @brief Read a state file: one list of numbers per name, e.g. `shots: [10]`;
 * a single number counts as a list of one. Throws if it cannot be read.
 */
Values readStateFile(const std::filesystem::path& path);

/**
 * @brief Write a state file, one list per name: to a temporary file first,
 * then renamed, so that a crash never leaves it half written. Creates its
 * folder if needed. Throws on failure.
 */
void writeStateFile(const std::filesystem::path& path, const Values& values);

/**
 * @brief The state of the rig that every page shares and the objectives use,
 * e.g. the marks of a focus stack, as the parameters `state.<name>` of this
 * node, each a list of numbers.
 *
 * Two kinds of values, by the parameters `saved` and `forgotten`:
 *
 * - **saved**, e.g. the counts of a stack that a page sets: kept in the YAML
 *   file `state_file`, so that they survive a restart. They start from the
 *   file, or from `defaults.<name>` while the file has none.
 * - **forgotten**, e.g. the marks of the rail, which are counts of motor steps
 *   and mean nothing once the motors' controller powers up again: in memory
 *   only. They start empty, `[]`, at every start of the node, which means
 *   "not set".
 *
 * Any client reads and sets them through the parameter services of the node,
 * e.g. SaveValues and LoadValues of the behaviors, or StepIt UI over
 * rosbridge, and follows them on /parameter_events. A name that is neither
 * saved nor forgotten does not exist. A value that cannot be saved is refused.
 * The node is the only one that writes the file.
 */
class StackState : public rclcpp::Node
{
public:
  explicit StackState(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

  /// @brief The file of the saved values, with `~` expanded.
  const std::filesystem::path& file() const
  {
    return file_;
  }

private:
  rcl_interfaces::msg::SetParametersResult onSetParameters(const std::vector<rclcpp::Parameter>& parameters);

  std::filesystem::path file_;
  std::vector<std::string> saved_;
  OnSetParametersCallbackHandle::SharedPtr callback_;
};

}  // namespace stepit_state
