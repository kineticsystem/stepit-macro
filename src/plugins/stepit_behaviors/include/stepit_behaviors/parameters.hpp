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

#include <string>

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
 *         state_file: ~/ws/state/stack.yaml
 *
 * The commander knows nothing about them: registerNodes declares them on its
 * node, so that they show in `ros2 param list` and can be read.
 */
void declareParameters(rclcpp::Node& node);

/// @brief The overshoot of a joint against backlash, `overshoot.<joint>`, in radians: 0 if not set.
double overshootParameter(rclcpp::Node& node, const std::string& joint);

/// @brief The YAML file where SaveValues and LoadValues keep their values, `state_file`, with `~` expanded.
std::string stateFileParameter(rclcpp::Node& node);

/// @brief The default of `state_file`.
inline constexpr auto kDefaultStateFile = "~/.ros/stepit_state.yaml";

}  // namespace stepit_behaviors
