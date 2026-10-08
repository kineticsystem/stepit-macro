// Small line icons, drawn with the text color.

const common = {
  width: 16, height: 16, viewBox: '0 0 16 16', fill: 'none', stroke: 'currentColor',
  strokeWidth: 1.5, strokeLinecap: 'round', strokeLinejoin: 'round', className: 'icon', 'aria-hidden': true,
} as const;

export function CameraIcon() {
  return (
    <svg {...common}>
      <path d="M2 5.5a1 1 0 0 1 1-1h2l1-1.5h4l1 1.5h2a1 1 0 0 1 1 1V12a1 1 0 0 1-1 1H3a1 1 0 0 1-1-1z" />
      <circle cx="8" cy="8.5" r="2.5" />
    </svg>
  );
}

export function PowerIcon() {
  return (
    <svg {...common}>
      <path d="M8 1.75v5.5" />
      <path d="M4.6 4a5 5 0 1 0 6.8 0" />
    </svg>
  );
}

export function GearIcon() {
  return (
    <svg {...common}>
      <circle cx="8" cy="8" r="2" />
      <path d="M8 1.5v2M8 12.5v2M1.5 8h2M12.5 8h2M3.4 3.4l1.4 1.4M11.2 11.2l1.4 1.4M3.4 12.6l1.4-1.4M11.2 4.8l1.4-1.4" />
    </svg>
  );
}

export function PlayIcon() {
  return (
    <svg {...common}>
      <path d="M5 3.5v9l7-4.5z" />
    </svg>
  );
}

export function StopIcon() {
  return (
    <svg {...common}>
      <rect x="4" y="4" width="8" height="8" rx="1" />
    </svg>
  );
}

export function ShutterIcon() {
  return (
    <svg {...common}>
      <circle cx="8" cy="8" r="6" />
      <circle cx="8" cy="8" r="2.5" />
    </svg>
  );
}

export function LightIcon() {
  return (
    <svg {...common}>
      <path d="M8 2a4 4 0 0 0-2.5 7.1V11h5V9.1A4 4 0 0 0 8 2z" />
      <path d="M6 13.5h4" />
    </svg>
  );
}

export function HandIcon() {
  return (
    <svg {...common}>
      <path d="M5 8V3.5a1 1 0 0 1 2 0V7m0-3.5V2.5a1 1 0 0 1 2 0V7m0-3a1 1 0 0 1 2 0v3.5m0-2a1 1 0 0 1 2 0V10a4 4 0 0 1-4 4H8a4 4 0 0 1-3.4-1.9L3 9.5a1 1 0 0 1 1.7-1L5 9" />
    </svg>
  );
}

export function VideoIcon() {
  return (
    <svg {...common}>
      <rect x="1.5" y="4" width="9" height="8" rx="1.5" />
      <path d="M10.5 7 14.5 4.5v7L10.5 9" />
    </svg>
  );
}

export function LayersIcon() {
  return (
    <svg {...common}>
      <path d="M8 2.5 14 5.5 8 8.5 2 5.5z" />
      <path d="M2 8.25 8 11.25 14 8.25" />
      <path d="M2 11 8 14 14 11" />
    </svg>
  );
}

/** The rotary stage: an arrow turning round. */
export function RotateIcon() {
  return (
    <svg {...common}>
      <path d="M13 8a5 5 0 1 1-1.5-3.55" />
      <path d="M11.5 1.75v2.75h-2.75" />
    </svg>
  );
}

/** The rail: an arrow up and down, as the slider moves the camera. */
export function RailIcon() {
  return (
    <svg {...common}>
      <path d="M8 2v12" />
      <path d="M5 4.75 8 2l3 2.75" />
      <path d="M5 11.25 8 14l3-2.75" />
    </svg>
  );
}

export function CheckIcon() {
  return (
    <svg {...common}>
      <path d="M3 8.5 6.5 12 13 4.5" />
    </svg>
  );
}
