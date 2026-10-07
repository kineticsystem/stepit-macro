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
#include <optional>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

namespace stepit_behaviors
{

/**
 * @brief The parameters of the behaviors, read from the node of the commander,
 * which loads them: the section of the commander in the robot's parameter
 * file, e.g.
 *
 *     stepit_server:
 *       ros__parameters:
 *         overshoot:
 *           joint2: 0.5
 *         mm_per_turn:
 *           joint2: 1.592
 *         deg_per_turn:
 *           joint1: 4.5
 *         focus_stack:
 *           turn: 17.0
 *         state_file: ~/ws/state/stack.yaml
 *         pictures_folder: ~/ws/pictures
 *
 * The commander knows nothing about them: registerNodes declares them on its
 * node, so that they show in `ros2 param list` and can be read, by the
 * behaviors and by the pages of the rig: `focus_stack.*` is read by StepIt UI
 * alone, the defaults of its stack. It also shows what the state file holds
 * as `state.<key>`, see publishState, and writes into the file every
 * `state.<key>` a page sets, e.g. the number of shots a page typed:
 * `state.turn`, `state.shots` and `state.angles` exist from the start, with
 * the defaults `focus_stack.turn`, `focus_stack.shots` and
 * `focus_stack.angles`.
 */
void declareParameters(rclcpp::Node& node);

/// @brief The overshoot of a joint against backlash, `overshoot.<joint>`, in radians: 0 if not set.
double overshootParameter(rclcpp::Node& node, const std::string& joint);

/// @brief How far a linear axis travels per turn of its motor, `mm_per_turn.<joint>`, in mm, if set.
std::optional<double> mmPerTurnParameter(rclcpp::Node& node, const std::string& joint);

/// @brief How far a rotary axis turns per turn of its motor, `deg_per_turn.<joint>`, in degrees, if set.
std::optional<double> degPerTurnParameter(rclcpp::Node& node, const std::string& joint);

/// @brief The YAML file where SaveValues and LoadValues keep their values, `state_file`, with `~` expanded.
std::string stateFileParameter(rclcpp::Node& node);

/**
 * @brief The camera's folder of pictures, `pictures_folder`, with `~`
 * expanded: the camera's download_directory, as seen by the commander, which
 * runs on the same computer. StackDone writes into the folders of the stacks
 * there.
 */
std::string picturesFolderParameter(rclcpp::Node& node);

/**
 * @brief Save numbers under a name in a YAML file, keeping the other names:
 * to a temporary file first, then renamed, so that a crash never leaves it
 * half written. Throws on failure.
 */
void writeStateValues(const std::filesystem::path& path, const std::string& key, const std::vector<double>& values);

/**
 * @brief Show values saved in the state file as the parameter `state.<key>`
 * of the commander's node, e.g. state.near, for the pages of the rig to read,
 * and to follow on /parameter_events: what one page marked, every page knows.
 * The node declares it the first time.
 */
void publishState(rclcpp::Node& node, const std::string& key, const std::vector<double>& values);

/// @brief The default of `state_file`.
inline constexpr auto kDefaultStateFile = "~/.ros/stepit_state.yaml";

/// @brief The default of `pictures_folder`: the camera's own default download_directory.
inline constexpr auto kDefaultPicturesFolder = "~/ws/pictures";

}  // namespace stepit_behaviors
