# Whether the gamepad is plugged in, published by gamepad_teleop on ~/status,
# e.g. /gamepad_teleop/status. Latched: published when it changes and every
# second, so that a subscriber that stops receiving it knows that the node is
# gone.

builtin_interfaces/Time stamp

# True while /joy keeps coming: joy_linux_node repeats the state of an open
# gamepad 20 times a second, even untouched, and says nothing once it is
# unplugged.
bool connected

# The joystick device, e.g. /dev/input/js0.
string device

# Why it is not connected; empty when it is.
string message
