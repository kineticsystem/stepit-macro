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

// stack_state, the node that keeps the state of the rig, on a state file of
// the test's own.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>
#include <stepit_state/stack_state.hpp>

namespace stepit_tests
{

class StackStateTest : public testing::Test
{
protected:
  void SetUp() override
  {
    if (!rclcpp::ok())
    {
      rclcpp::init(0, nullptr);
    }
    folder_ = std::filesystem::temp_directory_path() /
              ("stepit_stack_state_" + std::string(testing::UnitTest::GetInstance()->current_test_info()->name()));
    std::filesystem::remove_all(folder_);
    file_ = folder_ / "state" / "stack.yaml";
  }

  void TearDown() override
  {
    std::filesystem::remove_all(folder_);
  }

  /// @brief A node as the rig configures it, with these parameters too.
  std::shared_ptr<stepit_state::StackState> start(std::vector<rclcpp::Parameter> extra = {})
  {
    std::vector<rclcpp::Parameter> parameters{
      rclcpp::Parameter("state_file", file_.string()),
      rclcpp::Parameter("saved", std::vector<std::string>{ "turn", "shots", "angles" }),
      rclcpp::Parameter("forgotten", std::vector<std::string>{ "near", "far" }),
      rclcpp::Parameter("defaults.turn", 17.0),
      rclcpp::Parameter("defaults.shots", 10),
      rclcpp::Parameter("defaults.angles", 35),
    };
    parameters.insert(parameters.end(), extra.begin(), extra.end());
    rclcpp::NodeOptions options;
    options.parameter_overrides(parameters);
    return std::make_shared<stepit_state::StackState>(options);
  }

  void write(const std::string& text)
  {
    std::filesystem::create_directories(file_.parent_path());
    std::ofstream(file_) << text;
  }

  std::string read() const
  {
    std::ifstream in(file_);
    return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
  }

  static std::vector<double> value(const std::shared_ptr<stepit_state::StackState>& node, const std::string& name)
  {
    return node->get_parameter("state." + name).as_double_array();
  }

  std::filesystem::path folder_;
  std::filesystem::path file_;
};

// A new rig: the counts of a stack from the defaults, no mark.
TEST_F(StackStateTest, ANewRigStartsFromTheDefaults)
{
  const auto node = start();
  EXPECT_EQ(value(node, "turn"), (std::vector<double>{ 17.0 }));
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 10.0 }));
  EXPECT_EQ(value(node, "angles"), (std::vector<double>{ 35.0 }));
  EXPECT_TRUE(value(node, "near").empty());
  EXPECT_TRUE(value(node, "far").empty());
}

// What the file holds wins over the defaults; what it lacks keeps its default.
TEST_F(StackStateTest, TheSavedValuesComeBackAfterARestart)
{
  write("shots: [40]\nangles: [3]\n");
  const auto node = start();
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 40.0 }));
  EXPECT_EQ(value(node, "angles"), (std::vector<double>{ 3.0 }));
  EXPECT_EQ(value(node, "turn"), (std::vector<double>{ 17.0 }));
}

// The marks are counts of motor steps: a start forgets them, even those a
// file of an older rig still holds.
TEST_F(StackStateTest, AStartForgetsTheMarks)
{
  write("near: [12.5]\nfar: [15.0]\nshots: [40]\n");
  const auto node = start();
  EXPECT_TRUE(value(node, "near").empty());
  EXPECT_TRUE(value(node, "far").empty());
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 40.0 }));
}

TEST_F(StackStateTest, ASavedValueIsWrittenToTheFile)
{
  const auto node = start();
  ASSERT_TRUE(node->set_parameter(rclcpp::Parameter("state.shots", std::vector<double>{ 40.0 })).successful);
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 40.0 }));
  EXPECT_EQ(stepit_state::readStateFile(file_),
            (stepit_state::Values{ { "angles", { 35.0 } }, { "shots", { 40.0 } }, { "turn", { 17.0 } } }));
}

// The file is for people too: one list per name.
TEST_F(StackStateTest, TheFileIsPlainYaml)
{
  const auto node = start();
  ASSERT_TRUE(node->set_parameter(rclcpp::Parameter("state.shots", std::vector<double>{ 40.0 })).successful);
  EXPECT_EQ(read(), "angles: [35]\nshots: [40]\nturn: [17]\n");
}

TEST_F(StackStateTest, AMarkStaysInMemory)
{
  const auto node = start();
  ASSERT_TRUE(node->set_parameter(rclcpp::Parameter("state.near", std::vector<double>{ 12.5 })).successful);
  EXPECT_EQ(value(node, "near"), (std::vector<double>{ 12.5 }));
  EXPECT_FALSE(std::filesystem::exists(file_));
}

// A name it does not keep does not exist: its parameter services refuse it.
TEST_F(StackStateTest, ANameItDoesNotKeepCannotBeSet)
{
  const auto node = start();
  EXPECT_THROW(node->set_parameter(rclcpp::Parameter("state.nowhere", std::vector<double>{ 1.0 })),
               rclcpp::exceptions::ParameterNotDeclaredException);
}

TEST_F(StackStateTest, AValueMustBeAListOfNumbers)
{
  const auto node = start();
  EXPECT_FALSE(node->set_parameter(rclcpp::Parameter("state.shots", "forty")).successful);
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 10.0 }));
}

// A value that cannot be saved is refused, rather than lost at the next start.
TEST_F(StackStateTest, AValueThatCannotBeSavedIsRefused)
{
  // The folder of the file is a file: nothing can be written there.
  std::filesystem::create_directories(folder_);
  std::ofstream(folder_ / "state") << "";
  const auto node = start();
  EXPECT_FALSE(node->set_parameter(rclcpp::Parameter("state.shots", std::vector<double>{ 40.0 })).successful);
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 10.0 }));
}

// Where the file is, and what is kept or forgotten, is the rig's
// configuration: no client can change it while the rig runs.
TEST_F(StackStateTest, TheConfigurationIsReadOnly)
{
  const auto node = start();
  EXPECT_FALSE(node->set_parameter(rclcpp::Parameter("state_file", "/tmp/elsewhere.yaml")).successful);
  EXPECT_FALSE(node->set_parameter(rclcpp::Parameter("forgotten", std::vector<std::string>{ "near" })).successful);
  EXPECT_FALSE(node->set_parameter(rclcpp::Parameter("defaults.shots", 3)).successful);
}

// A broken file does not stop the rig: it starts from the defaults.
TEST_F(StackStateTest, ABrokenFileStartsFromTheDefaults)
{
  write("[not, a, map]\n");
  const auto node = start();
  EXPECT_EQ(value(node, "shots"), (std::vector<double>{ 10.0 }));
}

TEST_F(StackStateTest, AValueCannotBeBothSavedAndForgotten)
{
  EXPECT_THROW(start({ rclcpp::Parameter("forgotten", std::vector<std::string>{ "near", "shots" }) }),
               std::invalid_argument);
}

}  // namespace stepit_tests
