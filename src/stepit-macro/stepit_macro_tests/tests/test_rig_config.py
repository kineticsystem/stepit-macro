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


"""The configuration of the rig, rig.yaml, as rig.launch.py reads it, without starting anything."""

import importlib.util
import math
from pathlib import Path

import pytest
import yaml

# The sources of stepit_bringup, next to this package's.
BRINGUP = Path(__file__).resolve().parents[2] / "stepit_bringup"


def load_launch_file():
    path = BRINGUP / "launch" / "rig.launch.py"
    spec = importlib.util.spec_from_file_location("rig_launch", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


rig = load_launch_file()


def load_config():
    with open(BRINGUP / "config" / "rig.yaml") as file:
        return yaml.safe_load(file)


def test_rig_yaml_names_known_modules_only():
    arguments, _ = rig.split_config(load_config())
    assert set(arguments) <= set(rig.MODULES) | set(rig.PROGRAMS)


def test_rig_yaml_sets_the_serial_ports():
    arguments, _ = rig.split_config(load_config())
    assert "usb_port" in arguments["robot"]
    assert "usb_port" in arguments["freezer"]


def test_rig_yaml_parameters_are_a_ros_parameter_file():
    _, parameters = rig.split_config(load_config())
    assert "launch" not in parameters
    for node, section in parameters.items():
        assert set(section) == {"ros__parameters"}, node


def test_the_commander_loads_the_rig_behaviors_and_objectives():
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    assert commander["plugins"] == ["stepit_behaviors/bt_plugins"]
    assert commander["behavior_trees"] == ["stepit_objectives/objectives"]


def test_the_stage_overshoots_by_a_degree_and_the_rail_by_a_millimetre():
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    turns = {
        joint: radians / (2 * math.pi)
        for joint, radians in commander["overshoot"].items()
    }
    assert turns["joint1"] * commander["deg_per_turn"]["joint1"] == pytest.approx(
        1.0, abs=1e-4
    )
    assert turns["joint2"] * commander["mm_per_turn"]["joint2"] == pytest.approx(
        1.0, abs=1e-4
    )


def test_the_rail_has_its_measured_millimetres_per_turn():
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    assert commander["mm_per_turn"]["joint2"] == 1.592


def test_the_stage_turns_on_an_80_to_1_gear():
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    assert commander["deg_per_turn"]["joint1"] == 360 / 80


def test_a_stack_turns_the_stage_17_degrees_either_way_by_default():
    _, parameters = rig.split_config(load_config())
    state = parameters["stack_state"]["ros__parameters"]
    assert state["defaults"] == {"turn": 17.0, "shots": 10, "angles": 35}


def test_the_counts_survive_a_restart_and_the_marks_do_not():
    _, parameters = rig.split_config(load_config())
    state = parameters["stack_state"]["ros__parameters"]
    assert set(state["saved"]) == {"turn", "shots", "angles"}
    assert set(state["forgotten"]) == {"near", "far"}


def test_the_state_is_saved_in_the_state_folder():
    _, parameters = rig.split_config(load_config())
    state = parameters["stack_state"]["ros__parameters"]
    assert state["state_file"].startswith("~/ws/state/")


def test_the_commander_holds_no_state():
    # BehaviorTree.ROS2 registers the plugins and the trees again after any
    # change of the commander's parameters: state that changes while the rig
    # runs belongs to stack_state.
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    assert not {"state_file", "focus_stack", "state_cleared_on_start"} & set(commander)


def test_the_camera_and_its_web_server_share_the_pictures_folder():
    _, parameters = rig.split_config(load_config())
    camera = parameters["camera"]["ros__parameters"]
    web_server = parameters["web_server"]["ros__parameters"]
    assert camera["download_directory"] == web_server["download_directory"]


def test_the_commander_writes_into_the_cameras_pictures_folder():
    _, parameters = rig.split_config(load_config())
    commander = parameters["stepit_server"]["ros__parameters"]
    camera = parameters["camera"]["ros__parameters"]
    assert commander["pictures_folder"] == camera["download_directory"]


def test_an_unknown_module_is_refused():
    with pytest.raises(RuntimeError, match="unknown modules in `launch`: robto"):
        rig.split_config({"launch": {"robto": {"use_dummy": False}}})


def test_a_section_without_parameters_is_refused():
    with pytest.raises(RuntimeError, match="`freezer` is neither"):
        rig.split_config({"freezer": {"usb_port": "/dev/ttyUSB0"}})


def test_an_empty_file_starts_every_module_with_its_defaults():
    assert rig.split_config(None) == ({}, {})


@pytest.mark.parametrize(
    "value, argument",
    [
        (True, "true"),
        (False, "false"),
        (9600, "9600"),
        ("/dev/ttyACM0", "/dev/ttyACM0"),
    ],
)
def test_values_become_launch_arguments(value, argument):
    assert rig.to_argument(value) == argument


def test_the_editor_refuses_an_unknown_argument():
    with pytest.raises(RuntimeError, match="the editor has no argument prot"):
        rig.editor({"prot": 8080})


def test_the_ui_refuses_an_unknown_argument():
    with pytest.raises(RuntimeError, match="the ui has no argument prot"):
        rig.ui({"prot": 8070})


def test_the_ui_sliders_drive_the_stage_at_a_quarter_of_the_rail_speed():
    _, parameters = rig.split_config(load_config())
    teleop = parameters["ui_teleop"]["ros__parameters"]
    assert teleop["stop_button"] == -1
    assert teleop["joint1"] == {"axis": 0, "scale": pytest.approx(4.7124, abs=1e-3)}
    assert teleop["joint2"] == {"axis": 1, "scale": pytest.approx(18.8496, abs=1e-3)}
    for joint in ("joint3", "joint4", "joint5"):
        assert teleop[joint]["axis"] == -1


def test_the_freezer_fires_the_camera_on_out8_with_the_lights_on_out1():
    _, parameters = rig.split_config(load_config())
    freezer = parameters["freezer"]["ros__parameters"]
    assert freezer["default_sequence"] == "test_shot"
    assert freezer["sequences"]["test_shot"]["cameras"] == [8]
    assert freezer["sequences"]["test_shot"]["lights"] == [1]
