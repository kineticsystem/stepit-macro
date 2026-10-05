import { readFileSync, existsSync } from 'node:fs';
import { describe, expect, it } from 'vitest';
import { isJpeg, previewLocation } from '../src/camera/raw';
import { tiff } from './tiff';

describe('the preview inside a RAW', () => {
  const jpeg = [0xff, 0xd8, 1, 2, 3, 0xff, 0xd9];

  it('is the JPEG of the first image, in either byte order', () => {
    for (const little of [true, false]) {
      const file = tiff(jpeg, little);
      const at = previewLocation(file);
      expect(at).toEqual({ offset: 50, length: jpeg.length });
      expect(isJpeg(file.subarray(at!.offset, at!.offset + at!.length))).toBe(true);
    }
  });

  it('is missing from a file that is not a TIFF, or whose first image is not a JPEG', () => {
    expect(previewLocation(new Uint8Array([0xff, 0xd8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]))).toBeUndefined();
    const file = tiff([1, 2, 3]);
    const at = previewLocation(file)!;
    expect(isJpeg(file.subarray(at.offset, at.offset + at.length))).toBe(false);
  });

  it('is located from the start of the file only', () => {
    const file = tiff(jpeg);
    expect(previewLocation(file.subarray(0, 50))).toEqual({ offset: 50, length: jpeg.length });
    expect(previewLocation(file.subarray(0, 20))).toBeUndefined();
  });

  // A real CR2, when one is at hand: RAW_SAMPLE=/path/to/IMG_0001.CR2 pnpm test
  it.runIf(process.env.RAW_SAMPLE && existsSync(process.env.RAW_SAMPLE))('is found in a real CR2', () => {
    const file = new Uint8Array(readFileSync(process.env.RAW_SAMPLE!));
    const at = previewLocation(file.subarray(0, 64 * 1024))!;
    expect(at.length).toBeGreaterThan(100000);
    expect(isJpeg(file.subarray(at.offset, at.offset + at.length))).toBe(true);
  });
});
