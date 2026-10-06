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

#include "stepit_behaviors/set_joints.hpp"

#include <algorithm>
#include <vector>

#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{

SetJoints::SetJoints(const std::string& name, const BT::NodeConfig& config) : BT::SyncActionNode(name, config)
{
}

BT::PortsList SetJoints::providedPorts()
{
  return {
    BT::InputPort<std::vector<double>>("input", "the positions of every joint, in the order of joint_names"),
    BT::InputPort<std::vector<std::string>>("joint_names", "the joints of input, in its order"),
    BT::InputPort<BT::AnyTypeAllowed>("joints", "the joints to set: a name, or a list of names"),
    BT::InputPort<BT::AnyTypeAllowed>("values", "their values, in radians: one per joint, or one for all"),
    BT::InputPort<bool>("relative", false, "add the values to the positions instead of replacing them"),
    BT::OutputPort<std::vector<double>>("output", "the positions of every joint, these set"),
  };
}

BT::NodeStatus SetJoints::tick()
{
  const auto input = getInput<std::vector<double>>("input");
  if (!input)
  {
    throw BT::RuntimeError("SetJoints: ", input.error());
  }
  const auto joint_names = getNames(*this, "joint_names");
  const auto joints = getNames(*this, "joints");
  auto values = requireNumbers(*this, "values");
  const bool relative = getInput<bool>("relative").value_or(false);

  if (joint_names.size() != input->size())
  {
    throw BT::RuntimeError("SetJoints: [input] has ", std::to_string(input->size()), " positions for ",
                           std::to_string(joint_names.size()), " [joint_names]");
  }
  if (joints.empty())
  {
    throw BT::RuntimeError("SetJoints: [joints] must name at least one joint");
  }
  if (values.size() == 1 && joints.size() > 1)
  {
    values.assign(joints.size(), values.front());
  }
  if (values.size() != joints.size())
  {
    throw BT::RuntimeError("SetJoints: [values] has ", std::to_string(values.size()), " values for ",
                           std::to_string(joints.size()), " [joints]");
  }

  auto output = input.value();
  for (std::size_t i = 0; i < joints.size(); ++i)
  {
    const auto it = std::find(joint_names.cbegin(), joint_names.cend(), joints[i]);
    if (it == joint_names.cend())
    {
      throw BT::RuntimeError("SetJoints: joint '", joints[i], "' is not one of [joint_names]");
    }
    auto& position = output[static_cast<std::size_t>(std::distance(joint_names.cbegin(), it))];
    position = relative ? position + values[i] : values[i];
  }

  setOutput("output", output);
  return BT::NodeStatus::SUCCESS;
}

}  // namespace stepit_behaviors
