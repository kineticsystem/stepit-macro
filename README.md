# StepIt UI

> [!WARNING]
> This project is not started yet. Its first code, a web page to test the camera, moved to [StepIt Camera](https://github.com/kineticsystem/stepit-camera), whose test page it now is.

An automated macro photography system for 3D focus stacking.

A camera is mounted on a motorized linear rail, which sits on a rotary stage.
For each angle, the rail moves the camera through a series of focus distances
and captures an image at each step. The stage then rotates to the next angle
and repeats, producing a full set of focus stacks around the subject.

The system also switches LED lights on and off during the shoot, so lighting
is consistent and synchronized with each capture.

## Features

- Automated focus stacking along a linear rail
- Multi-angle capture via rotary stage
- Synchronized LED lighting control
- Output suitable for focus-stack merging and 3D reconstruction

## The Application

This repository will hold the application of the rig: the web page that runs a shoot, with one section per part of the rig, the camera, the rail, the rotary stage and the lights.

It starts again from scratch. Its history, a control panel of the camera, lives on in the test page of [StepIt Camera](https://github.com/kineticsystem/stepit-camera), in its `web` folder, from which the code that talks to the camera can be taken:

- `web/src/client/ros/`, a client of [rosbridge](https://github.com/RobotWebTools/rosbridge_suite), the WebSocket through which a browser calls ROS services and sets parameters;
- `web/src/client/camera/`, the camera driver's interface, and the loading of the pictures from its web server.
