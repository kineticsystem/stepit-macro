// A shot, and the pictures it brings. The shot is a task of the rig: the
// objective TakeShot, which StepIt Freezer fires through the camera's jack,
// with the lights. The camera driver then downloads the picture by itself, and
// tells of it on <camera>/picture; the page loads it from the camera's web
// server, and shows the latest in place of the live view.
//
// A shot stops the live view first: the two do not go together. A Canon EOS
// breaks its live view during a shot anyway, and the picture then takes the
// place of the live view. The page keeps the latest pictures only: see
// pictures.ts.

import { create } from 'zustand';
import { camera, useCamera } from '../camera/store';
import { Camera } from '../camera/camera';
import { loadPicture } from '../camera/picture';
import { useCommander } from '../commander/store';
import { errorMessage } from '../ros/rosbridge';
import { picturesUrl, useSettings } from '../settings';
import { release, withLoaded, withPicture, type Kept, type ShotPicture } from './pictures';

export type { ShotPicture };

/** How long a picture may take to reach the driver, after the shot ended. */
const PICTURE_TIMEOUT = 30000;
/** How long to wait for the other file of the same shot, e.g. the JPEG of RAW+JPEG. */
const SECOND_FILE_WAIT = 2000;

interface ShotState {
  state: 'idle' | 'shooting' | 'waiting' | 'done' | 'failed';
  message: string;
  /** The latest pictures, the latest first: KEPT_PICTURES of them. */
  pictures: ShotPicture[];
  takeShot(): Promise<void>;
}

const delay = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

export const useShot = create<ShotState>((set, get) => {
  /** Stores the pictures to keep, and releases the ones dropped. */
  const keep = (change: (pictures: ShotPicture[]) => Kept) => {
    const { pictures, dropped } = change(get().pictures);
    set({ pictures });
    dropped.forEach(release);
  };

  return {
    state: 'idle',
    message: '',
    pictures: [],

    async takeShot() {
      set({ state: 'shooting', message: 'Shooting…' });
      if (useCamera.getState().streaming) await useCamera.getState().setStreaming(false);
      let arrived = 0;
      let first: () => void = () => {};
      const came = new Promise<void>((resolve) => (first = resolve));
      // Listen before the shot, so that the picture cannot come first.
      const stop = camera().onPicture((picture) => {
        arrived++;
        first();
        const added: ShotPicture = {
          name: picture.name, path: picture.path,
          file: Camera.pictureUrl(picture.path, picturesUrl(useSettings.getState())),
        };
        keep((pictures) => withPicture(pictures, added));
        void load(added).then((loaded) => keep((pictures) => withLoaded(pictures, added, loaded)));
      });
      try {
        const result = await useCommander.getState().run('TakeShot');
        if (!result.ok) {
          set({ state: 'failed', message: `The shot failed: ${result.message}` });
          return;
        }
        if (arrived === 0) {
          set({ state: 'waiting', message: 'Downloading the picture…' });
          const got = await Promise.race([came.then(() => true), delay(PICTURE_TIMEOUT).then(() => false)]);
          if (!got) {
            set({
              state: 'failed',
              message: 'The Freezer fired the shot, but no picture came. Is the camera on, connected over USB, and plugged into OUT8?',
            });
            return;
          }
        }
        await delay(SECOND_FILE_WAIT);
        set({ state: 'done', message: '' });
      } catch (e) {
        set({ state: 'failed', message: errorMessage(e) });
      } finally {
        stop();
      }
    },
  };
});

async function load(picture: ShotPicture): Promise<ShotPicture> {
  try {
    return { ...picture, ...(await loadPicture(picture.file, picture.name)) };
  } catch (e) {
    return { ...picture, error: errorMessage(e) };
  }
}

/**
 * Saves a picture on this device, under its name. The camera's web server is
 * another origin than the page, where the download attribute of a link is
 * ignored: the file is fetched first, which its server allows.
 */
export async function download(picture: ShotPicture): Promise<void> {
  const response = await fetch(picture.file);
  if (!response.ok) throw new Error(`Cannot download ${picture.name}: ${response.status} ${response.statusText}`.trim());
  const url = URL.createObjectURL(await response.blob());
  const link = document.createElement('a');
  link.href = url;
  link.download = picture.name;
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(url), 10000);
}
