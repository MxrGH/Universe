#!/usr/bin/env python3
"""Convert a screen recording to an MP4 using an installed FFmpeg executable."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Input screen recording.")
    parser.add_argument("-o", "--output", type=Path, default=Path("Universe-Gameplay.mp4"))
    parser.add_argument("--fps", type=int, default=60)
    parser.add_argument("--crf", type=int, default=20, help="H.264 quality (lower is better; 18-28 is typical).")
    parser.add_argument("--preset", choices=["ultrafast", "fast", "medium", "slow"], default="medium")
    parser.add_argument("--start", help="Optional start time, e.g. 00:00:05.")
    parser.add_argument("--duration", help="Optional duration, e.g. 00:00:30.")
    args = parser.parse_args()

    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        print("FFmpeg was not found. Install FFmpeg and make sure it is on PATH.", file=sys.stderr)
        return 2
    if not args.input.is_file():
        print(f"Input file does not exist: {args.input}", file=sys.stderr)
        return 2
    if not 1 <= args.fps <= 240:
        parser.error("--fps must be between 1 and 240.")
    if not 0 <= args.crf <= 51:
        parser.error("--crf must be between 0 and 51.")

    command = [ffmpeg, "-hide_banner", "-y"]
    if args.start:
        command += ["-ss", args.start]
    command += ["-i", str(args.input)]
    if args.duration:
        command += ["-t", args.duration]
    command += [
        "-vf", f"fps={args.fps},format=yuv420p",
        "-c:v", "libx264",
        "-preset", args.preset,
        "-crf", str(args.crf),
        "-movflags", "+faststart",
        "-c:a", "aac",
        "-b:a", "192k",
        str(args.output),
    ]
    print("Running:", " ".join(command))
    result = subprocess.run(command, check=False)
    if result.returncode:
        print(f"FFmpeg exited with code {result.returncode}.", file=sys.stderr)
        return result.returncode
    print(f"Exported {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
