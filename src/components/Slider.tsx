import { useRef, type PointerEvent } from 'react';
import { axisAt, turnsPerSecond } from '../motion/axis';

/** The diameter of the knob, in pixels: a finger's width. Keep in step with styles.css. */
const KNOB = 56;

/**
 * A slider that works like a stick of a gamepad: the knob rests at the centre;
 * dragging it away asks for a speed, up to the motors' limit at the ends, one
 * way or the other; letting it go brings it back to the centre, and stops.
 *
 * It follows the pointer that grabbed it, so that two fingers drive two
 * sliders at once on a touch screen.
 */
export function Slider(props: {
  label: string;
  value: number;
  disabled: boolean;
  onChange(value: number): void;
}) {
  const track = useRef<HTMLDivElement>(null);
  const pointer = useRef<number | undefined>(undefined);

  const valueAt = (e: PointerEvent) => {
    const rect = track.current!.getBoundingClientRect();
    return axisAt(e.clientX, rect.left + KNOB / 2, rect.width - KNOB);
  };
  const down = (e: PointerEvent<HTMLDivElement>) => {
    if (props.disabled || pointer.current !== undefined) return;
    pointer.current = e.pointerId;
    e.currentTarget.setPointerCapture(e.pointerId);
    props.onChange(valueAt(e));
  };
  const move = (e: PointerEvent<HTMLDivElement>) => {
    if (e.pointerId === pointer.current) props.onChange(valueAt(e));
  };
  const up = (e: PointerEvent<HTMLDivElement>) => {
    if (e.pointerId !== pointer.current) return;
    pointer.current = undefined;
    props.onChange(0);
  };

  const speed = turnsPerSecond(props.value);
  return (
    <div className={`slider${props.disabled ? ' disabled' : ''}${props.value !== 0 ? ' moving' : ''}`}>
      <div className="slider-label">
        <span>{props.label}</span>
        <span className="mono">{speed === 0 ? 'stopped' : `${speed > 0 ? '+' : ''}${speed.toFixed(2)} turns/s`}</span>
      </div>
      <div
        ref={track}
        className="slider-track"
        onPointerDown={down}
        onPointerMove={move}
        onPointerUp={up}
        onPointerCancel={up}
        onLostPointerCapture={up}
      >
        <span className="slider-end">−</span>
        <span className="slider-centre" />
        <span className="slider-end">+</span>
        <span
          className="slider-knob"
          style={{ left: `calc(50% + ${props.value} * (50% - ${KNOB / 2}px))` }}
        />
      </div>
    </div>
  );
}
