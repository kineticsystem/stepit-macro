import { useState } from 'react';
import { useCommander } from '../commander/store';
import { useStatus } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { download, useShot, type ShotPicture } from '../shot/store';
import { DownloadIcon, ShutterIcon } from './icons';

/**
 * A shot, fired by StepIt Freezer through the objective TakeShot, and the
 * pictures of this session: the latest shown large, the others below.
 */
export function ShotCard() {
  const { state, message, pictures, takeShot } = useShot();
  const busy = useCommander((s) => s.busy);
  const connected = useStatus() === 'connected';
  const shooting = state === 'shooting' || state === 'waiting';
  const [latest, ...earlier] = pictures;

  return (
    <section className="card">
      <h2>Shot</h2>
      <button className="primary shutter" disabled={!connected || busy || shooting} onClick={() => void takeShot()}>
        <ShutterIcon />
        {shooting ? 'Shooting…' : 'Take a shot'}
      </button>
      {message && <p className={`message${state === 'failed' ? ' error' : ' muted'}`}>{message}</p>}
      {latest && <Picture picture={latest} large />}
      {earlier.length > 0 && (
        <ul className="pictures">
          {earlier.map((picture) => (
            <li key={picture.file}>
              <Picture picture={picture} />
            </li>
          ))}
        </ul>
      )}
    </section>
  );
}

function Picture({ picture, large = false }: { picture: ShotPicture; large?: boolean }) {
  const [error, setError] = useState<string>();
  const [saving, setSaving] = useState(false);
  const save = async () => {
    setSaving(true);
    setError(undefined);
    try {
      await download(picture);
    } catch (e) {
      setError(errorMessage(e));
    } finally {
      setSaving(false);
    }
  };

  return (
    <figure className={large ? 'shot large' : 'shot'}>
      {large && (picture.url ? (
        <a href={picture.url} target="_blank" rel="noreferrer">
          <img src={picture.url} alt={picture.name} />
        </a>
      ) : (
        <div className="shot-placeholder">
          {picture.error ?? (picture.size === undefined ? 'Loading…' : 'The browser cannot show this file')}
        </div>
      ))}
      <figcaption>
        <span className="shot-name">
          <strong>{picture.name}</strong>
          {picture.size !== undefined && <span className="muted"> {(picture.size / 1e6).toFixed(1)} MB</span>}
          {large && picture.preview && <span className="muted"> · its JPEG preview</span>}
        </span>
        <button onClick={() => void save()} disabled={saving} title={`Save ${picture.name} on this device`}>
          <DownloadIcon />
          {saving ? 'Saving…' : 'Download'}
        </button>
      </figcaption>
      {error && <p className="message error small">{error}</p>}
    </figure>
  );
}
