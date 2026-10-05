# Running the Rig on a Raspberry Pi 5

## Table of Contents <!-- omit in toc -->

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Install the Rig](#install-the-rig)
  - [Check the System](#check-the-system)
  - [Install Docker and Git](#install-docker-and-git)
  - [Connect StepIt Motors and StepIt Freezer](#connect-stepit-motors-and-stepit-freezer)
  - [Check out the Git Repository](#check-out-the-git-repository)
  - [Configure the Hardware](#configure-the-hardware)
  - [Build the Rig](#build-the-rig)
- [Running the Rig](#running-the-rig)
  - [Open the Pages](#open-the-pages)
  - [Start the Rig with the Pi](#start-the-rig-with-the-pi)
- [Logging in with an SSH Key](#logging-in-with-an-ssh-key)
- [Troubleshooting](#troubleshooting)

## Introduction

This document installs StepIt Macro on a Raspberry Pi 5, which then runs the whole rig: StepIt Motors, StepIt Freezer, the camera, the commander, the editor and StepIt UI, in the same container as on a PC. We control it from a tablet or a desktop on the same network, through StepIt UI.

The container brings its own Ubuntu 24.04 with ROS 2 Jazzy, whatever system the Pi runs. The Pi's system only provides the kernel, Docker and the drivers of the USB devices, which Raspberry Pi OS has: the serial ports of the Teensy (`cdc_acm`) and of the Arduino Nano (`ftdi_sio`), the camera and the gamepad.

Every command below runs on the Pi, in an SSH session:

```
ssh <user>@stepit.local
```

## Prerequisites

- A Raspberry Pi 5 with **8 GB** of memory, which the build needs, and **Raspberry Pi OS Lite (64-bit)**, reachable on the network as `stepit.local`. Lite has no desktop, which is what we want: a desktop grabs the camera as soon as it is plugged in, and the camera driver then cannot open it.
- The official 27 W power supply, and an active cooler: a long build throttles a Pi 5 without one.
- About 15 GB free on the SD card or, better, on an NVMe SSD: the image is about 3 GB, and the build writes a lot.
- StepIt Motors (the Teensy) and StepIt Freezer (the Arduino Nano), connected to the Pi's USB ports.

## Install the Rig

### Check the System

Check that the Pi runs the 64-bit system, has 8 GB of memory, and enough room on its disk:

```
uname -m
getconf PAGESIZE
free -h
df -h /
```

`uname -m` must print `aarch64`, and `free -h` about 7.9 Gi of memory. If it prints `armv7l`, the system is the 32-bit one, on which ROS 2 does not run: install Raspberry Pi OS Lite (64-bit) again. `getconf PAGESIZE` prints `16384` on a Pi 5: see [Troubleshooting](#troubleshooting) if a program crashes on the Pi and not on a PC.

### Install Docker and Git

Update the system, install git and Docker, and add our user to the groups `docker`, to run Docker without `sudo`, and `dialout`, to open the serial ports outside the container:

```
sudo apt update && sudo apt full-upgrade -y
sudo apt install -y git
curl -fsSL https://get.docker.com | sudo sh
sudo usermod -aG docker,dialout $USER
sudo reboot
```

The groups apply at the next login. After the reboot, log in again and check that Docker works without `sudo`:

```
docker run --rm hello-world
```

### Connect StepIt Motors and StepIt Freezer

Plug both boards into the Pi. If they were connected to a PC that ran the rig, stop the rig there first with `./docker/dock.sh stop`, so that the PC releases the ports. List the serial ports by their stable names:

```
ls -l /dev/serial/by-id/
```

The two boards appear under the same names as on any other computer, since the names come from their serial numbers:

| Board | Name | Usual port |
|---|---|---|
| StepIt Motors, the Teensy | `usb-Teensyduino_USB_Serial_12382150-if00` | `/dev/ttyACM0` |
| StepIt Freezer, the Arduino Nano | `usb-FTDI_FT232R_USB_UART_A700fkwd-if00-port0` | `/dev/ttyUSB0` |

The rig uses the names, not the ports: `/dev/ttyACM0` and `/dev/ttyUSB0` depend on the order the boards were plugged in.

The container needs nothing more to open them. It is privileged and mounts the whole of `/dev`, so it sees a board that is unplugged and plugged in again. Its user is in the group `dialout`, number 20, which owns the serial ports on Raspberry Pi OS as on Ubuntu.

ModemManager, when it is installed, probes every new `ttyACM` port, and can disturb the Teensy while it starts. Disable it if it runs; on Lite it is normally not installed, and the command does nothing:

```
systemctl is-active ModemManager && sudo systemctl disable --now ModemManager
```

### Check out the Git Repository

The repositories are public, so the Pi needs no GitHub key. `.gitmodules` lists the modules by their SSH addresses: the first command makes git fetch them over HTTPS instead. Check out the rig, with its modules and their own submodules:

```
git config --global url.https://github.com/.insteadOf git@github.com:
git clone --recurse-submodules https://github.com/kineticsystem/stepit-macro.git
cd stepit-macro
```

Later, to update the rig to the latest commit of `main`, with its modules, then build it again:

```
git pull && git submodule update --init --recursive
./docker/dock.sh build
```

### Configure the Hardware

The rig starts on fake hardware by default. Point it at the two boards, in [`rig.yaml`](../src/stepit-macro/stepit_bringup/config/rig.yaml):

```
sed -i \
  -e 's|use_dummy: true|use_dummy: false|' \
  -e 's|usb_port: /dev/ttyACM0|usb_port: /dev/serial/by-id/usb-Teensyduino_USB_Serial_12382150-if00|' \
  -e 's|use_fake: true|use_fake: false|' \
  -e 's|usb_port: /dev/ttyUSB0|usb_port: /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A700fkwd-if00-port0|' \
  src/stepit-macro/stepit_bringup/config/rig.yaml
```

Check the result:

```
grep -nE 'use_dummy|use_fake|usb_port' src/stepit-macro/stepit_bringup/config/rig.yaml
```

It shows `use_dummy: false`, `use_fake: false`, and the two names of the table above. This change is the Pi's own: do not commit it. See [Configuring the Rig](../README.md#configuring-the-rig) in the README for the rest of `rig.yaml`.

### Build the Rig

Build the image, then install the dependencies and compile every module in it:

```
./docker/dock.sh build
```

The first build takes 30 to 60 minutes on a Pi 5. The packages it downloads are kept by the package cache, `stepit-apt-cache`, so later builds are much faster.

## Running the Rig

Start the rig, and follow its output:

```
./docker/dock.sh start
./docker/dock.sh logs
```

`Ctrl+C` stops following the output, not the rig. The two boards are connected when the output shows:

```
Connection established with STEPIT controller, firmware 1.1.1.
Connection established with FREEZER controller, firmware 2.0.0.
```

Stop the rig with:

```
./docker/dock.sh stop
```

### Open the Pages

| Page | Address |
|---|---|
| StepIt UI | `http://stepit.local:8070` |
| StepIt Editor | `http://stepit.local:8080` |

Some Android tablets do not resolve `.local` names. Use the Pi's address instead, which this command prints:

```
hostname -I
```

Give the Pi a fixed address in the router's DHCP settings, so that the tablet always finds StepIt UI at the same place.

> [!WARNING]
> rosbridge, on port 9090, has no authentication: anyone on the network who reaches it can drive the rig. Keep the Pi on the workshop's own network.

### Start the Rig with the Pi

The rig does not start again when the Pi reboots. After the first start, ask Docker to restart it:

```
docker update --restart unless-stopped stepit-macro
```

Docker then starts the rig with the Pi, until we stop it with `./docker/dock.sh stop`. The setting lasts until the container is recreated, e.g. by `./docker/dock.sh clean`: run the command again after that.

## Logging in with an SSH Key

A key lets a PC log in to the Pi without its password: we type the password once, to install the key. Scripts and tools on the PC, e.g. a coding assistant checking the rig, can then run commands on the Pi. Run these commands on the PC, in a terminal, replacing `<user>` with the user name on the Pi.

Create a key of its own for the Pi, so that it can be withdrawn without touching the PC's other keys. It has no passphrase, so that nothing has to be typed when it is used; it only opens the Pi:

```
ssh-keygen -t ed25519 -f ~/.ssh/id_stepit -N "" -C "stepit-macro PC"
```

Install it on the Pi. This asks for the Pi's password, once:

```
ssh-copy-id -i ~/.ssh/id_stepit.pub <user>@stepit.local
```

Tell SSH to use it for the Pi, under the short name `stepit`:

```
cat >> ~/.ssh/config <<'EOF'

Host stepit stepit.local
    HostName stepit.local
    User <user>
    IdentityFile ~/.ssh/id_stepit
    IdentitiesOnly yes
EOF
chmod 600 ~/.ssh/config
```

Check it. This prints the Pi's name, without asking for a password:

```
ssh stepit hostname
```

To withdraw the access, delete the line of the key, which ends with `stepit-macro PC`, from `~/.ssh/authorized_keys` on the Pi, and the files `~/.ssh/id_stepit` and `~/.ssh/id_stepit.pub` on the PC.

## Troubleshooting

**A program crashes on the Pi but not on a PC.** Raspberry Pi OS runs the Pi 5 with memory pages of 16 KB, and a few arm64 programs only work with 4 KB pages. Switch to the standard kernel, with 4 KB pages, and reboot:

```
echo 'kernel=kernel8.img' | sudo tee -a /boot/firmware/config.txt
sudo reboot
```

`getconf PAGESIZE` then prints `4096`.

**The build stops with `Killed`, or the Pi stops answering during the build.** It ran out of memory, which happens on a Pi with less than 8 GB: the rig needs a Pi 5 with 8 GB.

**`ls -l /dev/serial/by-id/` does not list a board.** The Pi does not see it. Check the cable, which must carry data and not only power, and the board's LED. `journalctl -kf`, left running while the board is plugged in, prints what the kernel sees: a working Teensy ends with `ttyACM0: USB ACM device`, a working Nano with `now attached to ttyUSB0`.

**The log says the connection to a board failed.** Another program holds its port, e.g. a rig still running on a PC that shares the board, or ModemManager: see [Connect StepIt Motors and StepIt Freezer](#connect-stepit-motors-and-stepit-freezer). The Nano resets when its port opens: one failed attempt followed by `Connection established` is normal.

**The tablet cannot open `http://stepit.local:8070`.** Use the Pi's address, printed by `hostname -I`, see [Open the Pages](#open-the-pages).
