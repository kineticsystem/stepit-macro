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

#include <freezer_msgs/action/shoot.hpp>

#include "stepit_behaviors/ros_action_node.hpp"

namespace stepit_behaviors
{

/**
 * @brief Fires a shot on the StepIt Freezer board, and waits until it has ended.
 *
 * A thin wrapper around the Shoot action of the Freezer node: the board fires
 * the cameras, the flashes and the lights of a sequence, by the hardware timer
 * of its microcontroller. The sequence is named in the parameters of the
 * Freezer node, e.g. in rig.yaml; an empty name fires its default_sequence.
 *
 * A shot that has started cannot be stopped: the Freezer refuses to cancel it,
 * so that a camera's shutter always closes. Halting this node stops waiting for
 * it, and the shot runs to its end.
 */
class Shoot : public RosActionNode<freezer_msgs::action::Shoot>
{
public:
  Shoot(const std::string& name, const BT::NodeConfig& config, const BT::RosNodeParams& params);

  static BT::PortsList providedPorts();

  bool setGoal(Goal& goal) override;

  BT::NodeStatus onResultReceived(const WrappedResult& result) override;

  BT::NodeStatus onFailure(BT::ActionNodeErrorCode error) override;

  void onHalt() override;
};

}  // namespace stepit_behaviors
