# Copyright 2026 Giovanni Remigi
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


"""Drive the robot with a gamepad: joy_linux_node reads it, gamepad_teleop turns it into velocities."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "dev",
                default_value="/dev/input/js0",
                description="The joystick device of the gamepad.",
            ),
            DeclareLaunchArgument(
                "config",
                default_value=PathJoinSubstitution(
                    [
                        FindPackageShare("stepit_teleop"),
                        "config",
                        "logitech_dual_action.yaml",
                    ]
                ),
                description="The mapping of the gamepad: the stop button, and the axis of each joint.",
            ),
            # Waits for the gamepad, and opens it again when it is plugged back in.
            Node(
                package="joy_linux",
                executable="joy_linux_node",
                name="joy_linux_node",
                parameters=[
                    {
                        "dev": LaunchConfiguration("dev"),
                        "deadzone": 0.1,
                        # Repeat the state while nothing changes, so that
                        # gamepad_teleop can tell a held stick from a lost gamepad.
                        "autorepeat_rate": 20.0,
                    }
                ],
            ),
            Node(
                package="stepit_teleop",
                executable="gamepad_teleop",
                name="gamepad_teleop",
                parameters=[LaunchConfiguration("config")],
            ),
        ]
    )
