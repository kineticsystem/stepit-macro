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

#include "stepit_behaviors/millimetres_to_radians.hpp"

#include <cmath>
#include <vector>

#include "stepit_behaviors/parameters.hpp"
#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{

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
  const auto joint = getInput<std::string>("joint");
  if (!joint || joint.value().empty())
  {
    throw BT::RuntimeError("MillimetresToRadians: [joint] must name the joint of the axis");
  }
  const auto millimetres = requireNumbers(*this, "millimetres");
  const auto node = node_.lock();
  const auto mm_per_turn = node ? mmPerTurnParameter(*node, joint.value()) : std::nullopt;
  if (!mm_per_turn || mm_per_turn.value() == 0.0)
  {
    const auto logger = node ? node->get_logger() : rclcpp::get_logger("MillimetresToRadians");
    RCLCPP_ERROR(logger, "%s: no mm_per_turn.%s in the commander's parameters: measure it, and set it in rig.yaml",
                 name().c_str(), joint.value().c_str());
    return BT::NodeStatus::FAILURE;
  }

  std::vector<double> radians;
  radians.reserve(millimetres.size());
  for (const double mm : millimetres)
  {
    radians.push_back(mm / mm_per_turn.value() * 2.0 * M_PI);
  }
  setOutput("radians", radians);
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
