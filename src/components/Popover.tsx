import { useEffect, useRef, type ReactNode } from 'react';

/**
 * A button of the toolbar that opens a panel below it, e.g. the camera's
 * settings. A tap outside the panel, or Escape, closes it.
 */
export function Popover(props: {
  button: ReactNode;
  title: string;
  open: boolean;
  onOpenChange(open: boolean): void;
  /** Where the panel is anchored: to the left or to the right edge of the button. */
  align?: 'left' | 'right';
  children: ReactNode;
}) {
  const ref = useRef<HTMLDivElement>(null);
  const { open, onOpenChange } = props;

  useEffect(() => {
    if (!open) return;
    const close = (e: PointerEvent) => {
      if (!ref.current?.contains(e.target as Node)) onOpenChange(false);
    };
    const escape = (e: KeyboardEvent) => e.key === 'Escape' && onOpenChange(false);
    document.addEventListener('pointerdown', close);
    document.addEventListener('keydown', escape);
    return () => {
      document.removeEventListener('pointerdown', close);
      document.removeEventListener('keydown', escape);
    };
  }, [open, onOpenChange]);

  return (
    <div className="popover" ref={ref}>
      <button className={open ? 'active' : ''} aria-expanded={open} title={props.title} onClick={() => onOpenChange(!open)}>
        {props.button}
      </button>
      {open && (
        <div className={`popover-panel ${props.align ?? 'left'}`} role="dialog" aria-label={props.title}>
          {props.children}
        </div>
      )}
    </div>
  );
}
