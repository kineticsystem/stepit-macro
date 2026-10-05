// The pictures the page keeps: the latest few, for the page shows only the
// latest. The preview of a RAW is an object URL, which holds the JPEG in
// memory until it is revoked, so a picture the page drops is released.

/** How many pictures the page keeps: a shot in RAW+JPEG brings two. */
export const KEPT_PICTURES = 2;

export interface ShotPicture {
  name: string;
  path: string;
  /** The file on the camera's web server. */
  file: string;
  /** The size of the file, once loaded. */
  size?: number;
  /** What the browser can show, if it can: the JPEG itself, or an object URL of the preview inside a RAW. */
  url?: string;
  /** The URL shows the preview inside a RAW, not the picture itself. */
  preview?: boolean;
  /** Why the picture cannot be loaded. */
  error?: string;
}

/** The pictures to keep, and the ones dropped, to release. */
export interface Kept {
  pictures: ShotPicture[];
  dropped: ShotPicture[];
}

/** The pictures with a new one first, and the oldest beyond `kept` dropped. */
export function withPicture(pictures: ShotPicture[], added: ShotPicture, kept = KEPT_PICTURES): Kept {
  const all = [added, ...pictures];
  return { pictures: all.slice(0, kept), dropped: all.slice(kept) };
}

/**
 * The pictures with `added` replaced by what was loaded of it. A picture
 * dropped while it loaded stays dropped: what was loaded is to release.
 */
export function withLoaded(pictures: ShotPicture[], added: ShotPicture, loaded: ShotPicture): Kept {
  if (!pictures.includes(added)) return { pictures, dropped: [loaded] };
  return { pictures: pictures.map((p) => (p === added ? loaded : p)), dropped: [] };
}

/** Frees the memory of a picture's preview. A JPEG's URL is the file on the server: nothing to free. */
export function release(picture: ShotPicture): void {
  if (picture.url?.startsWith('blob:')) URL.revokeObjectURL(picture.url);
}
