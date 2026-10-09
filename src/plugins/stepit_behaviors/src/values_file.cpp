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

#include "stepit_behaviors/values_file.hpp"

#include <vector>

#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{
namespace
{

constexpr auto kStatePrefix = "state.";

std::string requireKey(const BT::TreeNode& node)
{
  const auto key = node.getInput<std::string>("key");
  if (!key || key.value().empty())
  {
    throw BT::RuntimeError(node.registrationName(), ": [key] must name the values");
  }
  return key.value();
}

}  // namespace

SaveValues::SaveValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::RosServiceNode<rcl_interfaces::srv::SetParameters>(name, config, params)
{
}

BT::PortsList SaveValues::providedPorts()
{
  return providedBasicPorts({
      BT::InputPort<std::string>("key", "the name to save the values under, e.g. near"),
      BT::InputPort<BT::AnyTypeAllowed>("values", "the numbers to save: a number, or a list"),
  });
}

bool SaveValues::setRequest(Request::SharedPtr& request)
{
  key_ = requireKey(*this);
  rcl_interfaces::msg::Parameter parameter;
  parameter.name = kStatePrefix + key_;
  parameter.value.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE_ARRAY;
  parameter.value.double_array_value = requireNumbers(*this, "values");
  request->parameters = { parameter };
  return true;
}

BT::NodeStatus SaveValues::onResponseReceived(const Response::SharedPtr& response)
{
  if (response->results.empty() || !response->results.front().successful)
  {
    RCLCPP_ERROR(logger(), "%s: cannot save %s: %s", name().c_str(), key_.c_str(),
                 response->results.empty() ? "no answer" : response->results.front().reason.c_str());
    return BT::NodeStatus::FAILURE;
  }
  RCLCPP_INFO(logger(), "%s: saved %s", name().c_str(), key_.c_str());
  return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus SaveValues::onFailure(BT::ServiceNodeErrorCode error)
{
  RCLCPP_ERROR(logger(), "%s: cannot save %s: %s, is stack_state running?", name().c_str(), key_.c_str(), toStr(error));
  return BT::NodeStatus::FAILURE;
}

LoadValues::LoadValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params)
  : BT::RosServiceNode<rcl_interfaces::srv::GetParameters>(name, config, params)
{
}

BT::PortsList LoadValues::providedPorts()
{
  return providedBasicPorts({
      BT::InputPort<std::string>("key", "the name the values were saved under, e.g. near"),
      BT::OutputPort<std::vector<double>>("values", "the numbers saved, always a list"),
  });
}

bool LoadValues::setRequest(Request::SharedPtr& request)
{
  key_ = requireKey(*this);
  request->names = { kStatePrefix + key_ };
  return true;
}

BT::NodeStatus LoadValues::onResponseReceived(const Response::SharedPtr& response)
{
  // A name the node does not know gets no value at all.
  if (response->values.empty() ||
      response->values.front().type != rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE_ARRAY)
  {
    RCLCPP_ERROR(logger(), "%s: the state of the rig has no %s", name().c_str(), key_.c_str());
    return BT::NodeStatus::FAILURE;
  }
  const auto& values = response->values.front().double_array_value;
  if (values.empty())
  {
    RCLCPP_ERROR(logger(), "%s: nothing saved as %s yet", name().c_str(), key_.c_str());
    return BT::NodeStatus::FAILURE;
  }
  setOutput("values", values);
  return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus LoadValues::onFailure(BT::ServiceNodeErrorCode error)
{
  RCLCPP_ERROR(logger(), "%s: cannot read %s: %s, is stack_state running?", name().c_str(), key_.c_str(), toStr(error));
  return BT::NodeStatus::FAILURE;
}

}  // namespace stepit_behaviors
