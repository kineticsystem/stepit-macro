import { afterEach, describe, expect, it, vi } from 'vitest';
import { KEPT_PICTURES, release, withLoaded, withPicture, type ShotPicture } from '../src/shot/pictures';

const picture = (name: string, url?: string): ShotPicture => ({ name, path: `/p/${name}`, file: `http://rig:8090/pictures/${name}`, url });

describe('the pictures the page keeps', () => {
  afterEach(() => vi.restoreAllMocks());

  it('are the latest few, the latest first, the others dropped', () => {
    let pictures: ShotPicture[] = [];
    const taken = ['IMG_0001.CR2', 'IMG_0001.JPG', 'IMG_0002.CR2'].map((name) => picture(name));
    const dropped: ShotPicture[] = [];
    for (const added of taken) {
      const kept = withPicture(pictures, added);
      pictures = kept.pictures;
      dropped.push(...kept.dropped);
    }
    expect(KEPT_PICTURES).toBe(2);
    expect(pictures.map((p) => p.name)).toEqual(['IMG_0002.CR2', 'IMG_0001.JPG']);
    expect(dropped.map((p) => p.name)).toEqual(['IMG_0001.CR2']);
  });

  it('take the place of their loading picture once loaded', () => {
    const added = picture('IMG_0001.CR2');
    const loaded = { ...added, size: 29e6, url: 'blob:preview', preview: true };
    expect(withLoaded([added], added, loaded)).toEqual({ pictures: [loaded], dropped: [] });
  });

  it('drop what was loaded of a picture dropped meanwhile', () => {
    const added = picture('IMG_0001.CR2');
    const newer = picture('IMG_0002.CR2');
    const loaded = { ...added, url: 'blob:preview' };
    expect(withLoaded([newer], added, loaded)).toEqual({ pictures: [newer], dropped: [loaded] });
  });

  it('free the preview of a RAW when released, and leave a JPEG on the server alone', () => {
    const revoke = vi.spyOn(URL, 'revokeObjectURL').mockImplementation(() => {});
    release(picture('IMG_0001.CR2', 'blob:http://rig:8070/1234'));
    release(picture('IMG_0001.JPG', 'http://rig:8090/pictures/IMG_0001.JPG'));
    release(picture('IMG_0002.CR2'));
    expect(revoke.mock.calls).toEqual([['blob:http://rig:8070/1234']]);
  });
});
