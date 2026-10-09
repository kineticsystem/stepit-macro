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

#include <optional>
#include <string>

#include <rclcpp/rclcpp.hpp>

namespace stepit_behaviors
{

/*
 * The parameters of the behaviors, read from the node of the commander, which
 * loads them: the section of the commander in the robot's parameter file, e.g.
 *
 *     stepit_server:
 *       ros__parameters:
 *         overshoot:
 *           joint2: 0.5
 *         mm_per_turn:
 *           joint2: 1.592
 *         deg_per_turn:
 *           joint1: 4.5
 *         pictures_folder: ~/ws/pictures
 *
 * The commander knows nothing about them, and they are not declared on its
 * node: they are read from the parameter file it was started with, its
 * parameter overrides. BehaviorTree.ROS2 registers every plugin and tree again
 * before the next goal after any change of the node's parameters, and a
 * declaration counts as one: declaring them while the plugin registers made
 * the first goal of every start register everything again, which drops the
 * objectives added since the last build. They are configuration, read-only.
 * What changes while the rig runs, e.g. the marks of a stack, is the state of
 * the rig, kept by the node stack_state, see SaveValues.
 */

/// @brief The overshoot of a joint against backlash, `overshoot.<joint>`, in radians: 0 if not set.
double overshootParameter(rclcpp::Node& node, const std::string& joint);

/// @brief How far a linear axis travels per turn of its motor, `mm_per_turn.<joint>`, in mm, if set.
std::optional<double> mmPerTurnParameter(rclcpp::Node& node, const std::string& joint);

/// @brief How far a rotary axis turns per turn of its motor, `deg_per_turn.<joint>`, in degrees, if set.
std::optional<double> degPerTurnParameter(rclcpp::Node& node, const std::string& joint);

/**
 * @brief The camera's folder of pictures, `pictures_folder`, with `~`
 * expanded: the camera's download_directory, as seen by the commander, which
 * runs on the same computer. StackDone writes into the folders of the stacks
 * there.
 */
std::string picturesFolderParameter(rclcpp::Node& node);

/// @brief The default of `pictures_folder`: the camera's own default download_directory.
inline constexpr auto kDefaultPicturesFolder = "~/ws/pictures";

}  // namespace stepit_behaviors
