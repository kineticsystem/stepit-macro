// A press that tells a tap from a hold: a hold does what must not happen by
// accident, e.g. marking an end of a stack, next to a slider that a thumb
// brushes at the end of a drag; a tap does what is harmless, e.g. moving back
// to the mark. The same for a finger, a mouse and a key.

/** How long a press must last to be a hold, in milliseconds. */
export const HOLD_MS = 600;

export interface Hold {
  /** The press begins: a pointer down, or Space or Enter down. */
  press(): void;
  /** The press ends: a tap, unless it has lasted long enough to be a hold already. */
  release(): void;
  /** The press is abandoned, e.g. the pointer left the button: neither a tap nor a hold. */
  cancel(): void;
}

export function createHold(handlers: { onHold(): void; onTap(): void; onChange?(holding: boolean): void },
  duration = HOLD_MS): Hold {
  let timer: ReturnType<typeof setTimeout> | undefined;
  const stop = () => {
    if (timer === undefined) return false;
    clearTimeout(timer);
    timer = undefined;
    handlers.onChange?.(false);
    return true;
  };
  return {
    press() {
      if (timer !== undefined) return;
      handlers.onChange?.(true);
      timer = setTimeout(() => {
        timer = undefined;
        handlers.onChange?.(false);
        handlers.onHold();
      }, duration);
    },
    release() {
      if (stop()) handlers.onTap();
    },
    cancel() {
      stop();
    },
  };
}
