// The sliders, sent to the rig as a gamepad: a sensor_msgs/Joy on /ui/joy, one
// axis per slider, which ui_teleop turns into velocities.
//
// While a slider is away from its centre, the axes are sent 20 times a second,
// even when they do not change: ui_teleop stops the joints when they stop
// coming for joy_timeout, 0.5 s, e.g. when the tablet loses the network in the
// middle of a move. Once every slider is back, they are sent once, at rest, and
// then no more: ui_teleop then leaves the velocity controller to others, e.g.
// the gamepad.

export const JOY_TOPIC = '/ui/joy';
export const JOY_TYPE = 'sensor_msgs/msg/Joy';
/** How often the axes are sent while a slider is held, in milliseconds. */
export const PERIOD = 50;

export type Publish = (topic: string, type: string, message: object) => void;

export class JoyPublisher {
  private readonly axes: number[];
  private timer?: ReturnType<typeof setInterval>;

  constructor(private readonly publish: Publish, axes: number) {
    this.axes = new Array<number>(axes).fill(0);
  }

  /** Sets an axis, and sends the axes at once if it changed. */
  set(axis: number, value: number): void {
    if (this.axes[axis] === value) return;
    this.axes[axis] = value;
    this.send();
    if (this.moving() && !this.timer) this.timer = setInterval(() => this.send(), PERIOD);
    if (!this.moving()) this.stopRepeating();
  }

  /** Every axis back to its centre, e.g. when the page is hidden. */
  release(): void {
    if (!this.moving()) return;
    this.axes.fill(0);
    this.send();
    this.stopRepeating();
  }

  private moving(): boolean {
    return this.axes.some((value) => value !== 0);
  }

  private send() {
    this.publish(JOY_TOPIC, JOY_TYPE, { header: { frame_id: 'stepit-ui' }, axes: [...this.axes], buttons: [] });
  }

  private stopRepeating() {
    clearInterval(this.timer);
    this.timer = undefined;
  }
}
