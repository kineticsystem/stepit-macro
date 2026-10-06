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

#include "stepit_behaviors/axis_units.hpp"

#include <cmath>
#include <optional>
#include <vector>

#include "stepit_behaviors/parameters.hpp"
#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{
namespace
{

using Ratio = std::optional<double> (*)(rclcpp::Node&, const std::string&);

/**
 * @brief Converts the values of `port`, in units of the axis of [joint], into
 * radians of its motor, with the ratio that `ratio` reads, in units per turn,
 * and writes them to [radians]. Fails, saying so, when the ratio is not set.
 */
BT::NodeStatus convert(BT::TreeNode& tree_node, const std::weak_ptr<rclcpp::Node>& weak, const std::string& port,
                       Ratio ratio, const char* parameter)
{
  const auto joint = tree_node.getInput<std::string>("joint");
  if (!joint || joint.value().empty())
  {
    throw BT::RuntimeError(tree_node.registrationName(), ": [joint] must name the joint of the axis");
  }
  const auto values = requireNumbers(tree_node, port);
  const auto node = weak.lock();
  const auto per_turn = node ? ratio(*node, joint.value()) : std::nullopt;
  if (!per_turn || per_turn.value() == 0.0)
  {
    const auto logger = node ? node->get_logger() : rclcpp::get_logger("stepit_behaviors");
    RCLCPP_ERROR(logger, "%s: no %s.%s in the commander's parameters: measure it, and set it in rig.yaml",
                 tree_node.name().c_str(), parameter, joint.value().c_str());
    return BT::NodeStatus::FAILURE;
  }

  std::vector<double> radians;
  radians.reserve(values.size());
  for (const double value : values)
  {
    radians.push_back(value / per_turn.value() * 2.0 * M_PI);
  }
  tree_node.setOutput("radians", radians);
  return BT::NodeStatus::SUCCESS;
}

}  // namespace

MillimetresToRadians::MillimetresToRadians(const std::string& name, const BT::NodeConfig& config,
                                           const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList MillimetresToRadians::providedPorts()
{
  return {
    BT::InputPort<std::string>("joint", "the joint of the linear axis, e.g. joint2"),
    BT::InputPort<BT::AnyTypeAllowed>("millimetres", "a distance along the axis, in mm: a number, or a list"),
    BT::OutputPort<std::vector<double>>("radians", "the same, in radians of the motor, always a list"),
  };
}

BT::NodeStatus MillimetresToRadians::tick()
{
  return convert(*this, node_, "millimetres", mmPerTurnParameter, "mm_per_turn");
}

DegreesToRadians::DegreesToRadians(const std::string& name, const BT::NodeConfig& config,
                                   const BT::RosNodeParams& params)
  : BT::SyncActionNode(name, config), node_(params.nh)
{
}

BT::PortsList DegreesToRadians::providedPorts()
{
  return {
    BT::InputPort<std::string>("joint", "the joint of the rotary axis, e.g. joint1"),
    BT::InputPort<BT::AnyTypeAllowed>("degrees", "an angle of the axis, in degrees: a number, or a list"),
    BT::OutputPort<std::vector<double>>("radians", "the same, in radians of the motor, always a list"),
  };
}

BT::NodeStatus DegreesToRadians::tick()
{
  return convert(*this, node_, "degrees", degPerTurnParameter, "deg_per_turn");
}

}  // namespace stepit_behaviors
