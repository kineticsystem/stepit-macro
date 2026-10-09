import { type CSSProperties, type KeyboardEvent, useMemo, useState } from 'react';
import { useCommander } from '../commander/store';
import { POWER_HOLD_MS, powerProblem } from '../power/power';
import { usePower } from '../power/store';
import { useStatus } from '../ros/connection';
import { createHold } from './hold';
import { PowerIcon } from './icons';

/**
 * Switches the rig's computer off, at the bottom of the settings menu, away
 * from the buttons a thumb uses all the time. **Hold for 3 seconds**: the
 * button fills while it is held, and letting go sooner cancels; a tap only
 * says to hold, in the top bar. Off while disconnected, and while any
 * objective runs, which power_off would refuse anyway: the reason shows under
 * it, as a tablet has no tooltip.
 */
export function PowerButton() {
  const connected = useStatus() === 'connected';
  const objective = useCommander((s) => s.objective);
  const switchingOff = usePower((s) => s.switchingOff);
  const [holding, setHolding] = useState(false);
  const problem = powerProblem(connected, objective, switchingOff);

  const hold = useMemo(
    () =>
      createHold(
        {
          onHold: () => void usePower.getState().switchOff(),
          onTap: () => usePower.getState().showHint(),
          onChange: setHolding,
        },
        POWER_HOLD_MS,
      ),
    [],
  );

  const key = (e: KeyboardEvent, down: boolean) => {
    if (e.key !== ' ' && e.key !== 'Enter') return;
    e.preventDefault();
    if (down) hold.press();
    else hold.release();
  };

  return (
    <div className="power-off">
      <button
        className={['hold', 'power', holding && 'holding'].filter(Boolean).join(' ')}
        style={{ '--hold-ms': `${POWER_HOLD_MS}ms` } as CSSProperties}
        disabled={problem !== undefined}
        title={problem ?? 'Hold for 3 seconds to switch the rig off'}
        onPointerDown={(e) => e.button === 0 && hold.press()}
        onPointerUp={() => hold.release()}
        onPointerCancel={() => hold.cancel()}
        onPointerLeave={() => hold.cancel()}
        onKeyDown={(e) => key(e, true)}
        onKeyUp={(e) => key(e, false)}
        onContextMenu={(e) => e.preventDefault()}
      >
        <PowerIcon />
        <span>Switch the rig off</span>
      </button>
      <p className="muted small">{problem ?? 'Hold for 3 seconds.'}</p>
    </div>
  );
}
