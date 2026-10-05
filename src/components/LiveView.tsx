import { useEffect, useState } from 'react';
import { Camera } from '../camera/camera';
import { useCamera } from '../camera/store';
import { useLights } from '../freezer/lights';
import { useShot } from '../shot/store';
import { useSettings, videoUrl } from '../settings';

/**
 * The live view, streamed by web_video_server as MJPEG into a plain <img>,
 * between the two sliders. The camera driver only sends frames while
 * streaming is on, which the toolbar starts and stops.
 *
 * During a shot the mirror goes down, and no frame comes until the picture is
 * downloaded: the last frame stays, greyed out. The messages of the shot and
 * of the lights show at the bottom of the picture.
 */
export function LiveView() {
  const { streaming, streamError } = useCamera();
  const { state, message } = useShot();
  const lightsError = useLights((s) => s.error);
  const shooting = state === 'shooting' || state === 'waiting';
  const node = useSettings((s) => s.cameraNode);
  const server = useSettings((s) => videoUrl(s));
  const [failed, setFailed] = useState(false);
  // Changing the key opens the stream again, e.g. after web_video_server restarted.
  const [attempt, setAttempt] = useState(0);
  const src = Camera.streamUrl(server, node);

  useEffect(() => setFailed(false), [src, streaming]);

  let overlay: string | undefined;
  if (streamError) overlay = streamError;
  else if (!streaming) overlay = 'The live view is off.';
  else if (failed) overlay = `Cannot reach web_video_server at ${server}.`;

  return (
    <section className={`viewport${shooting && streaming ? ' paused' : ''}`}>
      {streaming && !failed && (
        <img key={`${src}#${attempt}`} src={src} alt="What the camera sees" onError={() => setFailed(true)} />
      )}
      {overlay && (
        <div className="viewport-message">
          <p>{overlay}</p>
          {streaming && failed && (
            <button onClick={() => { setFailed(false); setAttempt(attempt + 1); }}>Retry</button>
          )}
        </div>
      )}
      {(message || lightsError) && (
        <div className="viewport-status">
          {message && <p className={state === 'failed' ? 'error' : ''}>{message}</p>}
          {lightsError && <p className="error">Lights: {lightsError}</p>}
        </div>
      )}
    </section>
  );
}
