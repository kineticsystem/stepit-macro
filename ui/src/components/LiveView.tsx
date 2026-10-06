import { useEffect, useState } from 'react';
import { Camera } from '../camera/camera';
import { useCamera } from '../camera/store';
import { useLights } from '../freezer/lights';
import { useSettings, videoUrl } from '../settings';
import { useShot, type ShotPicture } from '../shot/store';

/**
 * The middle of the page, between the two sliders: the live view while it is
 * on, and the last photo otherwise. We stream to frame the subject, and take
 * a picture with the live view off: a shot stops it, and its picture then
 * takes its place.
 *
 * The live view is an MJPEG stream of web_video_server in a plain <img>; the
 * camera driver only sends frames while streaming is on. The messages of the
 * shot and of the lights show over the bottom.
 */
export function LiveView() {
  const { streaming, streamError } = useCamera();
  const { state, message, pictures } = useShot();
  const lightsError = useLights((s) => s.error);
  const node = useSettings((s) => s.cameraNode);
  const server = useSettings((s) => videoUrl(s));
  const [failed, setFailed] = useState(false);
  // Changing the key opens the stream again, e.g. after web_video_server restarted.
  const [attempt, setAttempt] = useState(0);
  const src = Camera.streamUrl(server, node);
  const latest = pictures[0];

  useEffect(() => setFailed(false), [src, streaming]);

  let overlay: string | undefined;
  if (streaming) {
    if (streamError) overlay = streamError;
    else if (failed) overlay = `Cannot reach web_video_server at ${server}.`;
  } else if (!latest) {
    overlay = 'The live view is off.';
  }

  return (
    <section className="viewport">
      {streaming && !failed && !streamError && (
        <img key={`${src}#${attempt}`} src={src} alt="What the camera sees" onError={() => setFailed(true)} />
      )}
      {!streaming && latest && <Photo picture={latest} />}
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

/**
 * The last photo, as large as the viewport allows. Its file is in the camera
 * driver's folder of pictures, on the rig: the page neither names nor saves it.
 */
function Photo({ picture }: { picture: ShotPicture }) {
  return picture.url ? (
    <img src={picture.url} alt="The last photo" />
  ) : (
    <div className="viewport-message">
      <p>{picture.error ?? (picture.size === undefined ? 'Loading the photo…' : 'The browser cannot show this file')}</p>
    </div>
  );
}
