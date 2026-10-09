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

#include "stepit_behaviors/register_nodes.hpp"

#include "stepit_behaviors/axis_units.hpp"
#include "stepit_behaviors/command_joint_positions.hpp"
#include "stepit_behaviors/follow_joint_trajectory.hpp"
#include "stepit_behaviors/get_active_controllers.hpp"
#include "stepit_behaviors/get_joint_positions.hpp"
#include "stepit_behaviors/is_controller_active.hpp"
#include "stepit_behaviors/offset_vector.hpp"
#include "stepit_behaviors/picture_folder.hpp"
#include "stepit_behaviors/report_progress.hpp"
#include "stepit_behaviors/shoot.hpp"
#include "stepit_behaviors/stack_done.hpp"
#include "stepit_behaviors/cubic_trajectory.hpp"
#include "stepit_behaviors/expect_picture.hpp"
#include "stepit_behaviors/set_joints.hpp"
#include "stepit_behaviors/values_file.hpp"
#include "stepit_behaviors/steps.hpp"
#include "stepit_behaviors/switch_controller.hpp"

namespace stepit_behaviors
{

void registerNodes(BT::BehaviorTreeFactory& factory, const BT::RosNodeParams& params)
{
  factory.registerNodeType<OffsetVector>("OffsetVector");
  factory.registerNodeType<Steps>("Steps");
  factory.registerNodeType<CubicTrajectory>("CubicTrajectory");
  factory.registerNodeType<GetJointPositions>("GetJointPositions", params);
  factory.registerNodeType<FollowJointTrajectory>("FollowJointTrajectory", params);
  factory.registerNodeType<CommandJointPositions>("CommandJointPositions", params);
  factory.registerNodeType<GetActiveControllers>("GetActiveControllers", params);
  factory.registerNodeType<IsControllerActive>("IsControllerActive", params);
  factory.registerNodeType<SwitchController>("SwitchController", params);
  factory.registerNodeType<SetJoints>("SetJoints");
  factory.registerNodeType<MillimetresToRadians>("MillimetresToRadians", params);
  factory.registerNodeType<DegreesToRadians>("DegreesToRadians", params);
  // The state of the rig, the node stack_state, unless the tree names another service in service_name.
  BT::RosNodeParams save_params = params;
  save_params.default_port_value = std::string(kStateNode) + "/set_parameters";
  factory.registerNodeType<SaveValues>("SaveValues", save_params);
  BT::RosNodeParams load_params = params;
  load_params.default_port_value = std::string(kStateNode) + "/get_parameters";
  factory.registerNodeType<LoadValues>("LoadValues", load_params);
  factory.registerNodeType<ExpectPicture>("ExpectPicture", params);

  // The Freezer node's action, unless the tree names another in action_name.
  BT::RosNodeParams shoot_params = params;
  shoot_params.default_port_value = "/freezer/shoot";
  factory.registerNodeType<Shoot>("Shoot", shoot_params);

  // The camera node's parameters, unless the tree names another service in service_name.
  BT::RosNodeParams camera_params = params;
  camera_params.default_port_value = "/camera/set_parameters";
  factory.registerNodeType<SetPictureFolder>("SetPictureFolder", camera_params);
  factory.registerNodeType<CurrentTime>("CurrentTime");
  factory.registerNodeType<ReportProgress>("ReportProgress", params);

  // The topics of the finished stacks, created once, for every run: see latchedPublisher. Here,
  // after the other nodes: BehaviorTree.ROS2 calls registerNodes again after a change of the
  // commander's parameters, and the first registration then throws, as the node is registered
  // already, before any publisher is made.
  LatchedPublisher stack_done;
  LatchedPublisher all_stacks_done;
  if (const auto node = params.nh.lock())
  {
    stack_done = latchedPublisher(*node, kStackDoneTopic);
    all_stacks_done = latchedPublisher(*node, kAllStacksDoneTopic);
  }
  factory.registerNodeType<StackDone>("StackDone", params, stack_done);
  factory.registerNodeType<AllStacksDone>("AllStacksDone", params, all_stacks_done);
}

}  // namespace stepit_behaviors
