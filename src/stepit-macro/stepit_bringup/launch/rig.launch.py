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


"""Start the whole rig, every module with the configuration of config/rig.yaml."""

import os
import tempfile
from pathlib import Path

import yaml
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    GroupAction,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

# The repo, found from the source of this file, which build.sh installs as a
# link to it: src/stepit-macro/stepit_bringup/launch/rig.launch.py.
RIG_DIR = Path(__file__).resolve().parents[4]

# The launch file of each module, by its name in the section `launch` of
# rig.yaml, and whether it takes the node parameters of rig.yaml as params_file.
MODULES = {
    "robot": ("robot_bringup", "launch.py", False),
    "commander": ("stepit_server", "commander.launch.py", True),
    "camera": ("stepit_camera", "camera.launch.py", True),
    "freezer": ("freezer_node", "freezer.launch.py", True),
    "teleop": ("stepit_teleop", "teleop.launch.py", False),
}

# The arguments of the editor, which is not a ROS launch file.
EDITOR_ARGUMENTS = {"port"}


def split_config(config):
    """Split rig.yaml into the launch arguments of each module, and the node parameters."""
    config = dict(config or {})
    arguments = config.pop("launch", None) or {}
    unknown = set(arguments) - set(MODULES) - {"editor"}
    if unknown:
        raise RuntimeError(
            f"rig.yaml: unknown modules in `launch`: {', '.join(sorted(unknown))}"
        )
    for node, section in config.items():
        if not isinstance(section, dict) or "ros__parameters" not in section:
            raise RuntimeError(
                f"rig.yaml: `{node}` is neither `launch` nor a node with `ros__parameters`"
            )
    return arguments, config


def to_argument(value):
    """Write a value of rig.yaml as a launch argument: a YAML true is `true`, not `True`."""
    if isinstance(value, bool):
        return "true" if value else "false"
    return str(value)


def include(context, module, arguments, params_file):
    """Include the launch file of a module, with its arguments of rig.yaml only."""
    package, file, takes_params = MODULES[module]
    path = PathJoinSubstitution([FindPackageShare(package), "launch", file]).perform(
        context
    )
    source = PythonLaunchDescriptionSource(path)
    declared = {
        argument.name
        for argument in source.get_launch_description(context).get_launch_arguments()
    }
    unknown = set(arguments) - declared
    if unknown:
        raise RuntimeError(
            f"rig.yaml: {package}/{file} has no argument {', '.join(sorted(unknown))}"
        )
    values = {name: to_argument(value) for name, value in arguments.items()}
    if takes_params:
        values["params_file"] = params_file
    # A scoped group that forwards nothing: the arguments of one module, e.g.
    # usb_port or web_port, never reach the next.
    return GroupAction(
        [IncludeLaunchDescription(source, launch_arguments=values.items())],
        scoped=True,
        forwarding=False,
    )


def editor(arguments):
    """Serve the editor, on the rig's objectives, with the validator that build.sh built."""
    unknown = set(arguments) - EDITOR_ARGUMENTS
    if unknown:
        raise RuntimeError(
            f"rig.yaml: the editor has no argument {', '.join(sorted(unknown))}"
        )
    port = to_argument(arguments.get("port", 8080))
    directory = RIG_DIR / "modules/stepit-editor"
    # The command of the editor's `pnpm run start`, which serve.sh runs, without
    # pnpm: pnpm ignores the SIGINT that stops the rig, and is killed 5 seconds
    # later, while tsx stops the server at once.
    return ExecuteProcess(
        cmd=[str(directory / "node_modules/.bin/tsx"), "src/server/main.ts"],
        cwd=str(directory),
        name="stepit_editor",
        output="screen",
        # On the host network, PORT is the port of the host: EDITOR_PORT, the
        # port that the editor's own container publishes, is left unset.
        additional_env={
            "PORT": port,
            "BEHAVIORS_DIR": str(RIG_DIR / "src/plugins/stepit_objectives/objectives"),
            "BTCPP_VALIDATOR": str(RIG_DIR / "build/editor-validator/btcpp_validate"),
        },
    )


def launch_setup(context):
    with open(LaunchConfiguration("config").perform(context)) as file:
        arguments, parameters = split_config(yaml.safe_load(file))

    # The node parameters, written to a file of their own: a ROS2 parameter file
    # cannot hold the section `launch`, which is not a node.
    with tempfile.NamedTemporaryFile(
        "w", prefix="rig_parameters_", suffix=".yaml", delete=False
    ) as file:
        yaml.safe_dump(parameters, file)
        params_file = file.name

    actions = [
        include(context, module, arguments.get(module) or {}, params_file)
        for module in MODULES
    ]
    actions.append(editor(arguments.get("editor") or {}))
    return actions


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "config",
                default_value=os.path.join(
                    str(RIG_DIR), "src/stepit-macro/stepit_bringup/config/rig.yaml"
                ),
                description="The configuration of the rig.",
            ),
            OpaqueFunction(function=launch_setup),
        ]
    )
