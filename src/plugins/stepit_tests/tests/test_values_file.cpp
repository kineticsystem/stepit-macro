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

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <stepit_behaviors/register_nodes.hpp>

namespace stepit_behaviors::test
{

class ValuesFile : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    file_ = std::filesystem::temp_directory_path() /
            ("stepit_values_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + "_" +
             ::testing::UnitTest::GetInstance()->current_test_info()->name()) /
            "state.yaml";
    std::filesystem::remove_all(file_.parent_path());

    // The file comes from the commander's parameter state_file, as rig.yaml sets it.
    rclcpp::NodeOptions options;
    options.parameter_overrides({ rclcpp::Parameter("state_file", file_.string()) });
    node_ = std::make_shared<rclcpp::Node>("stepit_tests_values", options);
    stepit_behaviors::registerNodes(factory_, BT::RosNodeParams{ node_ });
  }

  void TearDown() override
  {
    std::filesystem::remove_all(file_.parent_path());
    node_.reset();
  }

  BT::NodeStatus tick(const std::string& node_xml, const BT::Blackboard::Ptr& blackboard)
  {
    auto tree = factory_.createTreeFromText(
        R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)" + node_xml + R"(</BehaviorTree></root>)", blackboard);
    return tree.tickOnce();
  }

  BT::NodeStatus save(const std::string& key, const std::vector<double>& values)
  {
    auto blackboard = BT::Blackboard::create();
    blackboard->set("values", values);
    return tick(R"(<SaveValues key=")" + key + R"(" values="{values}"/>)", blackboard);
  }

  /// @brief The values loaded, or nothing if LoadValues failed.
  std::optional<std::vector<double>> load(const std::string& key)
  {
    auto blackboard = BT::Blackboard::create();
    if (tick(R"(<LoadValues key=")" + key + R"(" values="{values}"/>)", blackboard) != BT::NodeStatus::SUCCESS)
    {
      return std::nullopt;
    }
    return blackboard->get<std::vector<double>>("values");
  }

  std::filesystem::path file_;
  rclcpp::Node::SharedPtr node_;
  BT::BehaviorTreeFactory factory_;
};

TEST_F(ValuesFile, LoadsWhatWasSaved)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  EXPECT_TRUE(std::filesystem::exists(file_));
  EXPECT_EQ(load("near"), (std::vector<double>{ 12.5 }));
}

TEST_F(ValuesFile, KeepsTheOtherNames)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(save("far", { 15.0, 1.0 }), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(save("near", { 13.0 }), BT::NodeStatus::SUCCESS);

  EXPECT_EQ(load("near"), (std::vector<double>{ 13.0 }));
  EXPECT_EQ(load("far"), (std::vector<double>{ 15.0, 1.0 }));
}

TEST_F(ValuesFile, FailsForANameNeverSaved)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(load("far"), std::nullopt);
}

TEST_F(ValuesFile, FailsWithoutAFile)
{
  EXPECT_EQ(load("near"), std::nullopt);
}

// The file is for people too: one list per name.
TEST_F(ValuesFile, IsPlainYaml)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  std::ifstream in(file_);
  const std::string text{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
  EXPECT_EQ(text, "near: [12.5]\n");
}

// The pages of the rig read what was saved as the commander's parameters, so
// that a mark set on one page shows on every page.
TEST_F(ValuesFile, WhatIsSavedIsShownAsAParameter)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(node_->get_parameter("state.near").as_double_array(), (std::vector<double>{ 12.5 }));

  ASSERT_EQ(save("near", { 13.0 }), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(node_->get_parameter("state.near").as_double_array(), (std::vector<double>{ 13.0 }));
}

// After a restart of the commander, the parameters show what the file holds.
TEST_F(ValuesFile, ACommanderStartingShowsWhatTheFileHolds)
{
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);
  ASSERT_EQ(save("far", { 15.0 }), BT::NodeStatus::SUCCESS);

  rclcpp::NodeOptions options;
  options.parameter_overrides({ rclcpp::Parameter("state_file", file_.string()) });
  const auto restarted = std::make_shared<rclcpp::Node>("stepit_tests_values_restarted", options);
  BT::BehaviorTreeFactory factory;
  stepit_behaviors::registerNodes(factory, BT::RosNodeParams{ restarted });

  EXPECT_EQ(restarted->get_parameter("state.near").as_double_array(), (std::vector<double>{ 12.5 }));
  EXPECT_EQ(restarted->get_parameter("state.far").as_double_array(), (std::vector<double>{ 15.0 }));
}

// A page sets the number of shots: it is saved in the file, as a mark is.
TEST_F(ValuesFile, WhatAPageSetsIsSavedInTheFile)
{
  // Declared by the first save.
  ASSERT_EQ(save("near", { 12.5 }), BT::NodeStatus::SUCCESS);

  ASSERT_TRUE(node_->set_parameter(rclcpp::Parameter("state.near", std::vector<double>{ 14.0 })).successful);
  EXPECT_EQ(load("near"), (std::vector<double>{ 14.0 }));
}

// The counts of a stack exist from the start, with rig.yaml's defaults, for a
// page to set.
TEST_F(ValuesFile, TheCountsOfAStackStartFromTheConfiguration)
{
  rclcpp::NodeOptions options;
  options.parameter_overrides({ rclcpp::Parameter("state_file", file_.string()),
                                rclcpp::Parameter("focus_stack.shots", 10),
                                rclcpp::Parameter("focus_stack.angles", 35) });
  const auto commander = std::make_shared<rclcpp::Node>("stepit_tests_values_counts", options);
  BT::BehaviorTreeFactory factory;
  stepit_behaviors::registerNodes(factory, BT::RosNodeParams{ commander });

  EXPECT_EQ(commander->get_parameter("state.shots").as_double_array(), (std::vector<double>{ 10.0 }));
  EXPECT_EQ(commander->get_parameter("state.angles").as_double_array(), (std::vector<double>{ 35.0 }));

  ASSERT_TRUE(commander->set_parameter(rclcpp::Parameter("state.shots", std::vector<double>{ 40.0 })).successful);
  EXPECT_EQ(load("shots"), (std::vector<double>{ 40.0 }));
}

}  // namespace stepit_behaviors::test
