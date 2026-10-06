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

#include "stepit_behaviors/picture_folder.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <vector>

#include "stepit_behaviors/ports.hpp"

namespace stepit_behaviors
{

CurrentTime::CurrentTime(const std::string& name, const BT::NodeConfig& config) : BT::SyncActionNode(name, config)
{
}

BT::PortsList CurrentTime::providedPorts()
{
  return {
    BT::InputPort<std::string>("format", "%Y-%m-%d_%H-%M-%S", "the format of strftime"),
    BT::OutputPort<std::string>("time", "the local time, formatted"),
  };
}

BT::NodeStatus CurrentTime::tick()
{
  const auto format = getInput<std::string>("format").value_or("%Y-%m-%d_%H-%M-%S");
  const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_r(&now, &local);
  char text[128];
  if (std::strftime(text, sizeof(text), format.c_str(), &local) == 0)
  {
    throw BT::RuntimeError("CurrentTime: [format] gives nothing, or more than 127 characters: ", format);
  }
  setOutput("time", std::string(text));
  return BT::NodeStatus::SUCCESS;
}

std::string angleFolder(int index, std::optional<double> degrees)
{
  char text[64];
  if (degrees)
  {
    std::snprintf(text, sizeof(text), "angle_%02d_%.1fdeg", index + 1, *degrees);
  }
  else
  {
    std::snprintf(text, sizeof(text), "angle_%02d", index + 1);
  }
  return text;
}

SetPictureFolder::SetPictureFolder(const std::string& name, const BT::NodeConfig& config,
                                   const BT::RosNodeParams& params)
  : BT::RosServiceNode<rcl_interfaces::srv::SetParameters>(name, config, params)
{
}

BT::PortsList SetPictureFolder::providedPorts()
{
  auto index = BT::InputPort<int>("index", "the angle of a stack, from 0: its folder inside [folder]");
  index.second.setDefaultValue(-1);
  return providedBasicPorts({
      BT::InputPort<std::string>("folder", "", "the folder under the camera's download_directory, e.g. tests"),
      index,
      optionalInput("degrees", "the angle in degrees, for the name of its folder"),
  });
}

bool SetPictureFolder::setRequest(Request::SharedPtr& request)
{
  folder_ = getInput<std::string>("folder").value_or("");
  const int index = getInput<int>("index").value_or(-1);
  if (index >= 0)
  {
    std::optional<double> degrees;
    if (isGiven(*this, "degrees"))
    {
      degrees = requireNumbers(*this, "degrees").front();
    }
    folder_ = (folder_.empty() ? "" : folder_ + "/") + angleFolder(index, degrees);
  }

  rcl_interfaces::msg::Parameter parameter;
  parameter.name = "folder";
  parameter.value.type = rcl_interfaces::msg::ParameterType::PARAMETER_STRING;
  parameter.value.string_value = folder_;
  request->parameters = { parameter };
  return true;
}

BT::NodeStatus SetPictureFolder::onResponseReceived(const Response::SharedPtr& response)
{
  if (response->results.empty() || !response->results.front().successful)
  {
    RCLCPP_ERROR(logger(), "%s: the camera refused the folder '%s': %s", name().c_str(), folder_.c_str(),
                 response->results.empty() ? "no answer" : response->results.front().reason.c_str());
    return BT::NodeStatus::FAILURE;
  }
  RCLCPP_INFO(logger(), "%s: the next pictures go into '%s'", name().c_str(), folder_.c_str());
  return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus SetPictureFolder::onFailure(BT::ServiceNodeErrorCode error)
{
  RCLCPP_ERROR(logger(), "%s: %s", name().c_str(), toStr(error));
  return BT::NodeStatus::FAILURE;
}

}  // namespace stepit_behaviors
