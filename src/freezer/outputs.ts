// The outputs of StepIt Freezer: 16 lines, two per jack. A jack's ring and tip
// are bits 2 * (jack - 1) + 1 and 2 * (jack - 1): OUT1 is bits 0 and 1, OUT8
// bits 14 and 15. A light closes both lines of its jack.

/** The jack of the lights on the rig. */
export const LIGHTS_JACK = 1;

/** The two lines of a jack, as bits of the outputs. */
export function jackBits(jack: number): number {
  return 0b11 << (2 * (jack - 1));
}

export function lightsOn(outputs: number, jack = LIGHTS_JACK): boolean {
  return (outputs & jackBits(jack)) === jackBits(jack);
}

/** The outputs with the lights on or off, and every other output as it is. */
export function withLights(outputs: number, on: boolean, jack = LIGHTS_JACK): number {
  return on ? outputs | jackBits(jack) : outputs & ~jackBits(jack) & 0xffff;
}
