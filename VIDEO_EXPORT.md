# Gameplay video export

Universe includes scripts/export_video.py, a helper for converting an existing gameplay recording to MP4.

## Requirements

- Python 3.9 or newer
- FFmpeg installed and available on PATH
- A screen recording captured with your preferred recording tool

## Example

~~~bash
python scripts/export_video.py recording.mkv --output Universe-Gameplay.mp4
~~~

Useful options:

- --fps 60 sets the output frame rate.
- --crf 20 controls H.264 quality (lower values produce larger, higher-quality files).
- --preset slow can reduce file size at the cost of encoding time.
- --start 00:00:10 --duration 00:00:30 exports a selected section.

The helper does not capture the game screen itself. Capture a recording first, then convert it with this script. FFmpeg is an external dependency and is not bundled with Universe.
