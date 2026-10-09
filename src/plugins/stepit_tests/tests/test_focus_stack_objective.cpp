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

// End to end tests of MarkNear, MarkFar and FocusStack: the XML shipped by
// stepit_objectives, run against a fake robot, a fake controller manager, a
// fake Freezer and a fake camera, with the overshoot of rig.yaml set as the
// commander's parameters.

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32_multi_array.hpp>
#include <std_msgs/msg/string.hpp>
#include <stepit_behaviors/register_nodes.hpp>

#include "fake/fake_camera.hpp"
#include "fake/fake_controller_manager.hpp"
#include "fake/fake_freezer.hpp"
#include "fake/fake_robot.hpp"
#include "fake/fake_state.hpp"
#include "objective.hpp"

namespace stepit_tests
{
namespace
{
constexpr auto kJointStateTopic = "/joint_states";
constexpr auto kActionName = "/joint_trajectory_controller/follow_joint_trajectory";
constexpr auto kCommandTopic = "/position_controller/commands";

const std::vector<std::string> kJointNames{ "joint1", "joint2", "joint3", "joint4", "joint5" };
/// The stage, joint1, at 0.5; the rail, joint2, at the near mark.
const std::vector<double> kJointPositions{ 0.5, 1.0, 0.0, -1.0, 2.0 };

constexpr double kStageOvershoot = 0.1;
constexpr double kRailOvershoot = 0.2;

/// 4 pi degrees of the stage per turn of its motor: 1 degree is 0.5 radians.
constexpr double kDegPerTurn = 4.0 * M_PI;

/// 3 shots from 1.0 to 2.0 on the rail, at 2 angles of the stage, 1 degree, 0.5 rad, either side of where it is.
constexpr auto kPayload = "{shots: 3, stage_from: -1, stage_to: 1, angles: 2}";

/// The stage and the rail of each command, rounded, to compare them.
std::vector<std::pair<double, double>> stageAndRail(const std::vector<std::vector<double>>& commands)
{
  std::vector<std::pair<double, double>> moves;
  for (const auto& command : commands)
  {
    moves.emplace_back(std::round(command[0] * 1000.0) / 1000.0, std::round(command[1] * 1000.0) / 1000.0);
  }
  return moves;
}
}  // namespace

class FocusStackObjective : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    folder_ = std::filesystem::temp_directory_path() /
              ("stepit_focus_stack_" + std::string(testing::UnitTest::GetInstance()->current_test_info()->name()));
    std::filesystem::remove_all(folder_);

    // The commander's section of rig.yaml.
    rclcpp::NodeOptions options;
    options.parameter_overrides({ rclcpp::Parameter("overshoot.joint1", kStageOvershoot),
                                  rclcpp::Parameter("overshoot.joint2", kRailOvershoot),
                                  rclcpp::Parameter("deg_per_turn.joint1", kDegPerTurn),
                                  rclcpp::Parameter("pictures_folder", pictures().string()) });
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_focus_stack", options);

    // The marks of the rail, not set, as at every start of the rig.
    state_ = std::make_unique<FakeState>();
    robot_ = std::make_unique<FakeRobot>(kJointStateTopic, kActionName, kJointNames, kJointPositions);
    robot_->followPositionCommands(kCommandTopic, 100.0);
    manager_ = std::make_unique<FakeControllerManager>(
        std::vector<FakeControllerManager::Controller>{ { "velocity_controller", "active", true },
                                                        { "position_controller", "inactive", true },
                                                        { "joint_state_broadcaster", "active", false } });
    camera_ = std::make_unique<FakeCamera>();
    camera_->saveTo(pictures());
    freezer_ = std::make_unique<FakeFreezer>(std::set<std::string>{ "test_shot" }, "test_shot",
                                             std::chrono::milliseconds{ 20 });
    freezer_->onShot([this]() { camera_->release(); });

    BT::RosNodeParams params;
    params.nh = node_;
    params.server_timeout = std::chrono::milliseconds{ 2000 };
    params.wait_for_server_timeout = std::chrono::milliseconds{ 2000 };
    stepit_behaviors::registerNodes(factory_, params);
    for (const auto* file : { "ensure_controllers.xml", "activate_teleop.xml", "mark_near.xml", "mark_far.xml",
                              "focus_stack.xml", "move_rail_to_mark.xml" })
    {
      factory_.registerBehaviorTreeFromFile(treePath("objectives", file).string());
    }
  }

  void TearDown() override
  {
    freezer_.reset();
    camera_.reset();
    manager_.reset();
    robot_.reset();
    state_.reset();
    node_.reset();
    std::filesystem::remove_all(folder_);
  }

  /// @brief The marks, as MarkNear and MarkFar would have saved them.
  void mark(double near, double far)
  {
    state_->set("near", { near });
    state_->set("far", { far });
  }

  /// @brief The camera's folder of pictures, in the test's own folder.
  std::filesystem::path pictures() const
  {
    return folder_ / "pictures";
  }

  /// @brief The text of a file, empty if there is none.
  static std::string read(const std::filesystem::path& path)
  {
    std::ifstream in(path);
    return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
  }

  /// @brief What FocusStack ended with, and what it announced.
  struct Run
  {
    BT::NodeStatus status;
    std::vector<std::string> stacks;      ///< On /focus_stack/stack_done.
    std::vector<std::string> all_stacks;  ///< On /focus_stack/all_stacks_done.
  };

  /// @brief Runs FocusStack, listening to the topics of the finished stacks, latched or not.
  Run runAndListen(bool latched = true)
  {
    std::mutex mutex;
    Run run;
    auto qos = rclcpp::QoS{ 10 }.reliable();
    if (latched)
    {
      qos.transient_local();
    }
    const auto listen = [&](const std::string& topic, std::vector<std::string>& into) {
      return node_->create_subscription<std_msgs::msg::String>(topic, qos,
                                                               [&mutex, &into](const std_msgs::msg::String& message) {
                                                                 const std::lock_guard<std::mutex> lock{ mutex };
                                                                 into.push_back(message.data);
                                                               });
    };
    const auto stacks = listen("/focus_stack/stack_done", run.stacks);
    const auto all_stacks = listen("/focus_stack/all_stacks_done", run.all_stacks);
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node_);
    std::thread spinner{ [&]() { executor.spin(); } };

    run.status = runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 90 });
    std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
    executor.cancel();
    spinner.join();
    const std::lock_guard<std::mutex> lock{ mutex };
    return run;
  }

  /// @brief The values saved in the state of the rig under `key`, or nothing.
  std::optional<std::vector<double>> saved(const std::string& key) const
  {
    const auto values = state_->get(key);
    return values.empty() ? std::nullopt : std::optional<std::vector<double>>{ values };
  }

  std::filesystem::path folder_;
  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<FakeState> state_;
  std::unique_ptr<FakeRobot> robot_;
  std::unique_ptr<FakeControllerManager> manager_;
  std::unique_ptr<FakeCamera> camera_;
  std::unique_ptr<FakeFreezer> freezer_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(FocusStackObjective, TheMarksSaveWhereTheRailIs)
{
  ASSERT_EQ(runObjective(factory_, "MarkNear", ""), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(saved("near"), (std::vector<double>{ 1.0 }));
  EXPECT_EQ(saved("far"), std::nullopt);

  ASSERT_EQ(runObjective(factory_, "MarkFar", ""), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(saved("far"), (std::vector<double>{ 1.0 }));
  EXPECT_EQ(saved("near"), (std::vector<double>{ 1.0 }));
}

// Marking does not switch the controllers: the gamepad keeps driving.
TEST_F(FocusStackObjective, MarkingLeavesTheControllersAlone)
{
  ASSERT_EQ(runObjective(factory_, "MarkNear", ""), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(manager_->lastSwitch().has_value());
  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
}

TEST_F(FocusStackObjective, ShootsAtEveryRailPositionOfEveryAngle)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(freezer_->fired().size(), 6u);
  EXPECT_EQ(camera_->taken(), 6);
  EXPECT_EQ(manager_->stateOf("position_controller"), "active");
}

// The pictures of a stack go into a folder named after when it started, with
// one folder per angle, its number and its angle; then back to the pictures
// folder itself.
TEST_F(FocusStackObjective, EachAngleHasAFolderOfPicturesInTheStacksFolder)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  const std::regex stack_folder{ R"(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})" };
  const auto stack = folders[0].substr(0, folders[0].find('/'));
  EXPECT_TRUE(std::regex_match(stack, stack_folder)) << stack;
  EXPECT_EQ(folders[0], stack + "/angle_01_-1.0deg");
  EXPECT_EQ(folders[1], stack + "/angle_02_1.0deg");
  EXPECT_EQ(folders[2], "");

  const auto pictures = camera_->pictures();
  ASSERT_EQ(pictures.size(), 6u);
  for (std::size_t i = 0; i < 6; ++i)
  {
    EXPECT_EQ(pictures[i].rfind(folders[i / 3] + "/", 0), 0u) << pictures[i];
  }
}

// Every page shows how far the stack is: the pictures taken, of how many, on a
// latched topic, whichever page started it.
TEST_F(FocusStackObjective, ItReportsThePicturesTakenOfHowMany)
{
  mark(1.0, 2.0);
  std::mutex mutex;
  std::vector<std::vector<int>> reports;
  const auto subscription = node_->create_subscription<std_msgs::msg::Int32MultiArray>(
      "/focus_stack/progress", rclcpp::QoS{ 10 }.reliable().transient_local(),
      [&](const std_msgs::msg::Int32MultiArray& message) {
        const std::lock_guard<std::mutex> lock{ mutex };
        reports.push_back({ message.data.begin(), message.data.end() });
      });
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_);
  std::thread spinner{ [&]() { executor.spin(); } };

  const auto status = runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 });
  std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
  executor.cancel();
  spinner.join();
  ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

  const std::lock_guard<std::mutex> lock{ mutex };
  ASSERT_EQ(reports.size(), 7u);
  for (int i = 0; i <= 6; ++i)
  {
    EXPECT_EQ(reports[static_cast<std::size_t>(i)], (std::vector<int>{ i, 6 }));
  }
}

// Once the last picture of an angle is saved, its folder is announced: its
// pictures are complete.
TEST_F(FocusStackObjective, EachAngleIsAnnouncedOnceItsPicturesAreSaved)
{
  mark(1.0, 2.0);
  const auto run = runAndListen();
  ASSERT_EQ(run.status, BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  EXPECT_EQ(run.stacks, (std::vector<std::string>{ folders[0], folders[1] }));
}

// A subscriber that is not latched, e.g. rosbridge's when it subscribed before
// the topics existed, hears every angle and the end of the stack too. In one
// process, a subscriber finds a new publisher at once, so this test passes
// even with publishers made at the first tick; across processes, through
// rosbridge, their first message was lost, which only a run on the rig shows.
TEST_F(FocusStackObjective, ASubscriberThatIsNotLatchedHearsEveryAnnouncement)
{
  mark(1.0, 2.0);
  const auto run = runAndListen(false);
  ASSERT_EQ(run.status, BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  EXPECT_EQ(run.stacks, (std::vector<std::string>{ folders[0], folders[1] }));
  EXPECT_EQ(run.all_stacks, (std::vector<std::string>{ std::filesystem::path(folders[0]).parent_path().string() }));
}

// Once the last angle is done, the stack's folder is announced: nothing more
// comes into it.
TEST_F(FocusStackObjective, TheStackIsAnnouncedOnceEveryAngleIsDone)
{
  mark(1.0, 2.0);
  const auto run = runAndListen();
  ASSERT_EQ(run.status, BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  const auto stack = std::filesystem::path(folders[0]).parent_path().string();
  EXPECT_EQ(run.all_stacks, (std::vector<std::string>{ stack }));
}

// The stack's folder says so itself, with all_stacks.json, which lists the
// folders of its angles.
TEST_F(FocusStackObjective, TheFinishedStackHasAnAllStacksFile)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  const auto stack = std::filesystem::path(folders[0]).parent_path();
  const auto file = read(pictures() / stack / "all_stacks.json");
  EXPECT_NE(file.find("\"folder\": \"" + stack.string() + "\""), std::string::npos) << file;
  EXPECT_NE(file.find("\"shots\": 3,"), std::string::npos) << file;
  EXPECT_NE(file.find("\"angles\": 2,"), std::string::npos) << file;
  EXPECT_NE(file.find("\"stacks\": [\n    \"" + std::filesystem::path(folders[0]).filename().string() + "\",\n    \"" +
                      std::filesystem::path(folders[1]).filename().string() + "\"\n  ]"),
            std::string::npos)
      << file;
}

// The folder of a finished angle says so itself, with stack.json, also to
// whoever reads the pictures later.
TEST_F(FocusStackObjective, EachFinishedAngleHasAStackFile)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  const auto folders = camera_->folders();
  ASSERT_EQ(folders.size(), 3u);
  const auto first = read(pictures() / folders[0] / "stack.json");
  EXPECT_NE(first.find("\"folder\": \"" + folders[0] + "\""), std::string::npos) << first;
  EXPECT_NE(first.find("\"shots\": 3,"), std::string::npos) << first;
  EXPECT_NE(first.find("\"angle\": 1,"), std::string::npos) << first;
  EXPECT_NE(first.find("\"degrees\": -1,"), std::string::npos) << first;
  EXPECT_NE(first.find("\"files\": [\n    \"IMG_1.CR2\",\n    \"IMG_2.CR2\",\n    \"IMG_3.CR2\"\n  ]"),
            std::string::npos)
      << first;
  const auto second = read(pictures() / folders[1] / "stack.json");
  EXPECT_NE(second.find("\"angle\": 2,"), std::string::npos) << second;
  EXPECT_NE(second.find("\"IMG_6.CR2\""), std::string::npos) << second;
}

// A stack that stops halfway is not complete: nothing is announced, and its
// folders have neither stack.json nor all_stacks.json.
TEST_F(FocusStackObjective, AStackThatStopsAnnouncesNothing)
{
  mark(1.0, 2.0);
  camera_->ignore(1);
  const auto run = runAndListen();
  EXPECT_EQ(run.status, BT::NodeStatus::FAILURE);

  EXPECT_TRUE(run.stacks.empty());
  EXPECT_TRUE(run.all_stacks.empty());
  if (std::filesystem::exists(pictures()))
  {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(pictures()))
    {
      EXPECT_NE(entry.path().filename(), "stack.json") << entry.path();
      EXPECT_NE(entry.path().filename(), "all_stacks.json") << entry.path();
    }
  }
}

// Every move approaches its position from the start of the stack: the rail
// from near to far, the stage upward. The stage starts at 0.5, so its angles
// are 0.0 and 1.0.
TEST_F(FocusStackObjective, EveryPositionIsApproachedTheSameWay)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  const std::vector<std::pair<double, double>> expected{
    // To the start: the stage comes down to 0.0, against the approach, and
    // the rail is already at the near mark, maybe driven there either way:
    // both go past, and back.
    { -0.1, 0.8 },
    { 0.0, 1.0 },
    // The first angle: the rail steps toward the far mark, already there for the first shot.
    { 0.0, 1.0 },
    { 0.0, 1.5 },
    { 0.0, 2.0 },
    // The second angle: the stage turns on, with the approach; the rail comes
    // back to the near mark, against it: past, and back.
    { 1.0, 0.8 },
    { 1.0, 1.0 },
    { 1.0, 1.5 },
    { 1.0, 2.0 },
    // Back to the start.
    { 0.5, 1.0 },
  };
  EXPECT_EQ(stageAndRail(robot_->positionCommands()), expected);
}

TEST_F(FocusStackObjective, TheOtherJointsStayInPlace)
{
  mark(1.0, 2.0);
  ASSERT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 60 }), BT::NodeStatus::SUCCESS);

  for (const auto& command : robot_->positionCommands())
  {
    EXPECT_DOUBLE_EQ(command[2], 0.0);
    EXPECT_DOUBLE_EQ(command[3], -1.0);
    EXPECT_DOUBLE_EQ(command[4], 2.0);
  }
}

// The camera ignored a release, as the real one sometimes does: the whole
// stack stops at once, without firing the shot again, rather than leave a gap.
TEST_F(FocusStackObjective, StopsWhenTheCameraIgnoresAShot)
{
  mark(1.0, 2.0);
  camera_->ignore(1);
  EXPECT_EQ(runObjective(factory_, "FocusStack", kPayload, std::chrono::seconds{ 90 }), BT::NodeStatus::FAILURE);

  EXPECT_EQ(freezer_->fired().size(), 1u);
  EXPECT_EQ(camera_->taken(), 0);
}

TEST_F(FocusStackObjective, NeedsBothMarks)
{
  state_->set("near", { 1.0 });
  EXPECT_EQ(runObjective(factory_, "FocusStack", kPayload), BT::NodeStatus::FAILURE);

  EXPECT_TRUE(robot_->positionCommands().empty());
  EXPECT_TRUE(freezer_->fired().empty());
}

// Back to a mark, to check the focus there: the rail approaches it as a stack
// does, from the near mark toward the far one, then manual drive again.
TEST_F(FocusStackObjective, TheRailGoesBackToAMarkAsAStackApproachesIt)
{
  mark(1.0, 2.0);

  // From 1.0 to the far mark, 2.0, with the approach: straight.
  ASSERT_EQ(runObjective(factory_, "MoveRailToMark", "{mark: far}"), BT::NodeStatus::SUCCESS);
  // Back to the near mark, against the approach: past it, then to it.
  ASSERT_EQ(runObjective(factory_, "MoveRailToMark", "{mark: near}"), BT::NodeStatus::SUCCESS);

  const std::vector<std::pair<double, double>> expected{
    { 0.5, 2.0 },
    { 0.5, 1.0 - kRailOvershoot },
    { 0.5, 1.0 },
  };
  EXPECT_EQ(stageAndRail(robot_->positionCommands()), expected);
  EXPECT_EQ(manager_->stateOf("velocity_controller"), "active");
}

// The state keeps other values next to the marks: none of them is a mark.
TEST_F(FocusStackObjective, OnlyNearAndFarAreMarks)
{
  // As the rig's stack_state keeps it: the shots next to the marks.
  state_.reset();
  state_ = std::make_unique<FakeState>(
      std::map<std::string, std::vector<double>>{ { "near", {} }, { "far", {} }, { "shots", { 25.0 } } });
  mark(1.0, 2.0);
  for (const auto* payload : { "{mark: shots}", "{mark: middle}" })
  {
    EXPECT_EQ(runObjective(factory_, "MoveRailToMark", payload), BT::NodeStatus::FAILURE) << payload;
  }
  // A number is not even a name: the tree is not created, and the commander aborts the goal.
  EXPECT_ANY_THROW(runObjective(factory_, "MoveRailToMark", "{mark: 1.5}"));
  EXPECT_TRUE(robot_->positionCommands().empty());
}

TEST_F(FocusStackObjective, GoingToAMarkNeedsBothMarks)
{
  state_->set("near", { 1.0 });
  EXPECT_EQ(runObjective(factory_, "MoveRailToMark", "{mark: near}"), BT::NodeStatus::FAILURE);
  EXPECT_TRUE(robot_->positionCommands().empty());
}

}  // namespace stepit_tests
