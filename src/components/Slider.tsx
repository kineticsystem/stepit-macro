import { useRef, type PointerEvent } from 'react';
import { axisAt, turnsPerSecond } from '../motion/axis';

/** The diameter of the knob, in pixels: a thumb's width. Keep in step with styles.css. */
const KNOB = 64;

/**
 * A vertical slider that works like a stick of a gamepad, for a thumb at the
 * edge of a tablet: the knob rests at the centre; pushing it up or down asks
 * for a speed, up to the slider's speed at the ends; letting it go brings it
 * back to the centre, and stops.
 *
 * It follows the pointer that grabbed it, so that two thumbs drive the two
 * sliders at once on a touch screen.
 */
export function Slider(props: {
  label: string;
  /** The speed at the ends, in turns per second. */
  maxTurnsPerSecond: number;
  value: number;
  disabled: boolean;
  onChange(value: number): void;
}) {
  const track = useRef<HTMLDivElement>(null);
  const pointer = useRef<number | undefined>(undefined);

  const valueAt = (e: PointerEvent) => {
    const rect = track.current!.getBoundingClientRect();
    return axisAt(e.clientY, rect.top + KNOB / 2, rect.height - KNOB);
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

  const speed = turnsPerSecond(props.value, props.maxTurnsPerSecond);
  return (
    <div className={`slider${props.disabled ? ' disabled' : ''}${props.value !== 0 ? ' moving' : ''}`}>
      <div className="slider-label">{props.label}</div>
      <div
        ref={track}
        className="slider-track"
        onPointerDown={down}
        onPointerMove={move}
        onPointerUp={up}
        onPointerCancel={up}
        onLostPointerCapture={up}
      >
        <span className="slider-end">+</span>
        <span className="slider-centre" />
        <span className="slider-end">−</span>
        <span
          className="slider-knob"
          style={{ top: `calc(50% - ${props.value} * (50% - ${KNOB / 2}px))` }}
        />
      </div>
      <div className="slider-speed mono">
        {speed === 0 ? 'stopped' : `${speed > 0 ? '+' : ''}${speed.toFixed(2)}`}
        {speed !== 0 && <span className="muted"> turns/s</span>}
      </div>
    </div>
  );
}
