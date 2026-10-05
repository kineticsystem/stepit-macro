import { useEffect, useRef, useState } from 'react';
import { picturesUrl, rosbridgeUrl, useSettings, videoUrl, type Theme } from '../settings';
import { CameraSettings } from './CameraSettings';
import { GearIcon } from './icons';

/**
 * The settings, under the gear, by how often they change: the camera's,
 * before a shoot; the theme of this browser; and, rarely, where the rig's
 * servers are.
 */
export function SettingsMenu() {
  const [open, setOpen] = useState(false);
  const ref = useRef<HTMLDivElement>(null);
  const settings = useSettings();

  useEffect(() => {
    if (!open) return;
    const close = (e: PointerEvent) => {
      if (!ref.current?.contains(e.target as Node)) setOpen(false);
    };
    const escape = (e: KeyboardEvent) => e.key === 'Escape' && setOpen(false);
    document.addEventListener('pointerdown', close);
    document.addEventListener('keydown', escape);
    return () => {
      document.removeEventListener('pointerdown', close);
      document.removeEventListener('keydown', escape);
    };
  }, [open]);

  return (
    <div className="settings" ref={ref}>
      <button className={`icon-button${open ? ' active' : ''}`} title="Settings" onClick={() => setOpen(!open)}>
        <GearIcon />
      </button>
      {open && (
        <div className="settings-popover">
          <CameraSettings />
          <h2 className="settings-group">Appearance</h2>
          <label className="setting">
            <span className="setting-label">Theme</span>
            <select value={settings.theme} onChange={(e) => settings.update({ theme: e.target.value as Theme })}>
              <option value="auto">Auto</option>
              <option value="light">Light</option>
              <option value="dark">Dark</option>
            </select>
          </label>
          <h2 className="settings-group">Connection</h2>
          <p className="muted small">Empty for the computer that serves this page.</p>
          <UrlField
            label="rosbridge"
            value={settings.rosbridgeUrl}
            placeholder={rosbridgeUrl({ rosbridgeUrl: '' })}
            onChange={(rosbridgeUrl) => settings.update({ rosbridgeUrl })}
          />
          <UrlField
            label="Live view"
            value={settings.videoUrl}
            placeholder={videoUrl({ videoUrl: '' })}
            onChange={(videoUrl) => settings.update({ videoUrl })}
          />
          <UrlField
            label="Pictures"
            value={settings.picturesUrl}
            placeholder={picturesUrl({ picturesUrl: '' })}
            onChange={(picturesUrl) => settings.update({ picturesUrl })}
          />
          <UrlField
            label="Camera node"
            value={settings.cameraNode}
            placeholder="/camera"
            onChange={(cameraNode) => settings.update({ cameraNode: cameraNode.trim() || '/camera' })}
          />
        </div>
      )}
    </div>
  );
}

/** A text field that applies its value when it loses the focus or on Enter, not on every key. */
function UrlField(props: { label: string; value: string; placeholder: string; onChange(value: string): void }) {
  const [text, setText] = useState(props.value);
  useEffect(() => setText(props.value), [props.value]);
  const apply = () => text !== props.value && props.onChange(text);
  return (
    <label className="setting-field">
      <span className="setting-label">{props.label}</span>
      <input
        className="mono"
        value={text}
        placeholder={props.placeholder}
        spellCheck={false}
        onChange={(e) => setText(e.target.value)}
        onBlur={apply}
        onKeyDown={(e) => e.key === 'Enter' && apply()}
      />
    </label>
  );
}
