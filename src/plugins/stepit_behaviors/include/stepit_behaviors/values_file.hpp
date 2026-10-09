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

#include <behaviortree_ros2/bt_service_node.hpp>
#include <rcl_interfaces/srv/get_parameters.hpp>
#include <rcl_interfaces/srv/set_parameters.hpp>

namespace stepit_behaviors
{

/// @brief The node that keeps the state of the rig, see stepit_state's StackState.
inline constexpr auto kStateNode = "/stack_state";

/**
 * @brief Saves numbers under a name in the state of the rig, e.g. where the
 * user marked the near end of a stack, for LoadValues to read back in a later
 * objective, and for every page to show.
 *
 * The state is the node stack_state, which keeps each value as its parameter
 * `state.<key>`: set here through its set_parameters service. The node knows
 * which names exist, and which it keeps across restarts, e.g. the counts of a
 * stack, or forgets at every start, e.g. the marks. A name it does not know,
 * or a value it cannot save, fails the node, saying why.
 */
class SaveValues : public BT::RosServiceNode<rcl_interfaces::srv::SetParameters>
{
public:
  SaveValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  bool setRequest(Request::SharedPtr& request) override;

  BT::NodeStatus onResponseReceived(const Response::SharedPtr& response) override;

  BT::NodeStatus onFailure(BT::ServiceNodeErrorCode error) override;

private:
  std::string key_;
};

/**
 * @brief Reads the numbers saved under a name in the state of the rig, the
 * parameter `state.<key>` of stack_state, through its get_parameters service.
 * Fails, saying so, when the name does not exist, or nothing is saved under it
 * yet, e.g. when the user has not marked that end of the stack.
 */
class LoadValues : public BT::RosServiceNode<rcl_interfaces::srv::GetParameters>
{
public:
  LoadValues(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  bool setRequest(Request::SharedPtr& request) override;

  BT::NodeStatus onResponseReceived(const Response::SharedPtr& response) override;

  BT::NodeStatus onFailure(BT::ServiceNodeErrorCode error) override;

private:
  std::string key_;
};

}  // namespace stepit_behaviors
