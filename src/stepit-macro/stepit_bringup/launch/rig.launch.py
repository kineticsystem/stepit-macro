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
    LogInfo,
    OpaqueFunction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
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

# The programs that are not ROS launch files, by their name in the section
# `launch` of rig.yaml, and the arguments each takes.
PROGRAMS = {"editor": {"port"}, "ui": {"port"}}


def split_config(config):
    """Split rig.yaml into the launch arguments of each module, and the node parameters."""
    config = dict(config or {})
    arguments = config.pop("launch", None) or {}
    unknown = set(arguments) - set(MODULES) - set(PROGRAMS)
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


def check_program(program, arguments):
    """Refuse an argument that a program does not take."""
    unknown = set(arguments) - PROGRAMS[program]
    if unknown:
        raise RuntimeError(
            f"rig.yaml: the {program} has no argument {', '.join(sorted(unknown))}"
        )


def editor(arguments):
    """Serve the editor, on the rig's objectives, with the validator that build.sh built."""
    check_program("editor", arguments)
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


def ui(arguments):
    """Serve StepIt UI, as built by build.sh into ui/dist."""
    check_program("ui", arguments)
    port = to_argument(arguments.get("port", 8070))
    directory = RIG_DIR / "ui/dist"
    if not (directory / "index.html").is_file():
        return LogInfo(msg=f"No StepIt UI in {directory}: build it with build.sh.")
    # Static files only: the page talks to the rig through rosbridge and the
    # servers of the camera.
    return ExecuteProcess(
        cmd=["python3", "-m", "http.server", port, "--directory", str(directory)],
        name="stepit_ui",
        output="log",
    )


def ui_teleop(params_file):
    """Drive the joints from the sliders of StepIt UI, as the gamepad does.

    A second gamepad_teleop, which reads the sliders on /ui/joy instead of the
    gamepad on /joy, with its parameters in the section `ui_teleop` of rig.yaml.
    Its watchdog stops the joints when the page stops sending, e.g. when the
    tablet loses the network in the middle of a move.
    """
    return Node(
        package="stepit_teleop",
        executable="gamepad_teleop",
        name="ui_teleop",
        output="screen",
        parameters=[params_file],
        remappings=[("/joy", "/ui/joy")],
    )


def clear_state(parameters):
    """Forget what the rig must not remember across a start, e.g. the marks of a stack.

    The section stepit_server of rig.yaml names the state file, state_file, and the
    entries to remove from it, state_cleared_on_start: the marks of the rail are
    counts of motor steps, which mean nothing once the motors' controller has
    powered up again, as it does when the Pi is switched on. The rest, e.g. the
    counts of a stack, is kept. The commander never sees state_cleared_on_start:
    it is the rig's own, and is taken out of the parameters. Returns the entries
    removed.
    """
    server = (parameters.get("stepit_server") or {}).get("ros__parameters") or {}
    keys = server.pop("state_cleared_on_start", None) or []
    path = server.get("state_file")
    if not keys or not path:
        return []
    path = Path(os.path.expanduser(path))
    if not path.exists():
        return []
    state = yaml.safe_load(path.read_text()) or {}
    cleared = [key for key in keys if key in state]
    if not cleared:
        return []
    for key in cleared:
        del state[key]
    # Written aside, then renamed: a crash never leaves it half written.
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(yaml.safe_dump(state, default_flow_style=None))
    temporary.replace(path)
    return cleared


def launch_setup(context):
    with open(LaunchConfiguration("config").perform(context)) as file:
        arguments, parameters = split_config(yaml.safe_load(file))

    # Before anything starts: the commander reads the state file when it does.
    cleared = clear_state(parameters)

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
    if cleared:
        actions.insert(
            0,
            LogInfo(msg=f"Cleared from the state file at start: {', '.join(cleared)}"),
        )
    actions.append(ui_teleop(params_file))
    actions.append(editor(arguments.get("editor") or {}))
    actions.append(ui(arguments.get("ui") or {}))
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
