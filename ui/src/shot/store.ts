// A shot, and the pictures it brings. The shot is a task of the rig: the
// objective TakeShot, which StepIt Freezer fires through the camera's jack,
// with the lights. The camera driver then downloads the picture by itself, and
// tells of it on <camera>/picture; the page loads it from the camera's web
// server, and shows the latest in place of the live view. The objective ends
// once the picture came, and fails when none does: the picture proves the
// shot, on the rig, for every client.
//
// A shot stops the live view first: the two do not go together. A Canon EOS
// breaks its live view during a shot anyway, and the picture then takes the
// place of the live view. The page keeps the latest pictures only: see
// pictures.ts.

import { create } from 'zustand';
import { camera, useCamera } from '../camera/store';
import { Camera, type Picture } from '../camera/camera';
import { loadPicture } from '../camera/picture';
import { useCommander } from '../commander/store';
import { onConnected } from '../ros/connection';
import { errorMessage } from '../ros/rosbridge';
import { picturesUrl, useSettings } from '../settings';
import { release, withLoaded, withPicture, type Kept, type ShotPicture } from './pictures';

export type { ShotPicture };

interface ShotState {
  state: 'idle' | 'shooting' | 'done' | 'failed';
  message: string;
  /** The latest pictures, the latest first: KEPT_PICTURES of them. */
  pictures: ShotPicture[];
  takeShot(): Promise<void>;
  /** Shows a picture the camera reported in place of the live view, e.g. one of a stack. */
  show(picture: Picture): void;
}

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
      try {
        // followPictures shows the picture, as every page shows every picture.
        const result = await useCommander.getState().run('TakeShot');
        set(result.ok ? { state: 'done', message: '' } : {
          state: 'failed',
          message: `The shot failed, or no picture came: is the camera on, connected over USB, and plugged into OUT8? (${result.message})`,
        });
      } catch (e) {
        set({ state: 'failed', message: errorMessage(e) });
      }
    },

    show(picture) {
      const added: ShotPicture = {
        name: picture.name, path: picture.path,
        file: Camera.pictureUrl(picture, picturesUrl(useSettings.getState())),
      };
      keep((pictures) => withPicture(pictures, added));
      void load(added).then((loaded) => keep((pictures) => withLoaded(pictures, added, loaded)));
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
 * Shows every picture the camera takes in place of the live view, whoever
 * fired it: a test shot or a stack started from any page or device. Returns
 * a function that stops it.
 */
export function followPictures(): () => void {
  let stop = () => {};
  const follow = () => {
    stop();
    stop = camera().onPicture((picture) => useShot.getState().show(picture));
  };
  follow();
  const unsubscribe = onConnected(follow);
  return () => {
    unsubscribe();
    stop();
  };
}
