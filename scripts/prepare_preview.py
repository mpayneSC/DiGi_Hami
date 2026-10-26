"""Create a display-sized GIF set without modifying the source GIFs.

Requires Pillow: python -m pip install Pillow
Run: python scripts/prepare_preview.py
"""
from pathlib import Path
from PIL import Image, ImageOps, ImageFilter
import colorsys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "device_data"
SOURCES = {
    "idle.gif": "idle.gif",
    "drinking.gif": "drinking.gif",
    "eating.gif": "eating.gif",
    "exercising.gif": "exercising.gif",
    "sleeping.gif": "sleeping.gif",
    "thumbs-up.gif": "thumbs-up.gif",
}
# Stay below 75% of the 0x2F0000 SPIFFS partition, including metadata headroom.
BUDGET = 2200000


def lighten_idle(frame):
    # Warm brown pixels only: retain dark eyes and the neutral bedding/background.
    pixels = []
    for red, green, blue in frame.getdata():
        hue, saturation, value = colorsys.rgb_to_hsv(red / 255, green / 255, blue / 255)
        warm = max(0.0, 1.0 - abs(hue - 0.075) / 0.10)
        brown = warm * min(1.0, max(0.0, (saturation - 0.16) / 0.30))
        brown *= min(1.0, value / 0.20) * min(1.0, max(0.0, (0.92 - value) / 0.30))
        # Lift shadows toward a lighter honey brown; never turn near-black eyes tan.
        lift = 0.30 * brown
        pixels.append((int(red + (255 - red) * lift),
                       int(green + (235 - green) * lift),
                       int(blue + (200 - blue) * lift)))
    frame.putdata(pixels)
    return frame


def prepare(source):
    image = Image.open(source)
    frames, durations = [], []
    elapsed, next_sample = 0, 0
    for index in range(image.n_frames):
        image.seek(index)
        duration = max(image.info.get("duration", 40), 10)
        if source.name == "idle.gif":
            # The reverse half needs extra flash. Keep idle at a steady 20 FPS
            # while retaining elapsed duration and leaving other scenes untouched.
            if frames and elapsed < next_sample:
                durations[-1] += duration
                elapsed += duration
                continue
            next_sample += 50
            elapsed += duration
        frame = ImageOps.pad(image.convert("RGB"), (160, 160),
                             method=Image.Resampling.LANCZOS, color="white")
        # Suppress source GIF grain, improving compression without dropping motion.
        frame = frame.filter(ImageFilter.MedianFilter(3))
        if source.name == "idle.gif":
            frame = lighten_idle(frame)
        frames.append(frame)
        durations.append(duration)
    if source.name == "idle.gif":
        # Avoid duplicate endpoint frames, which would pause at each turnaround.
        frames = frames + frames[-2:0:-1]
        durations = durations + durations[-2:0:-1]
    return frames, durations


OUTPUT.mkdir(exist_ok=True)
prepared = {name: prepare(ROOT / "data" / source) for name, source in SOURCES.items()}
for colors in (128, 96, 64, 48, 32):
    total = 0
    for name, (frames, durations) in prepared.items():
        strip = Image.new("RGB", (160, 160 * len(frames)))
        for i, frame in enumerate(frames):
            strip.paste(frame, (0, 160 * i))
        palette = strip.quantize(colors=min(colors, 32) if name == "idle.gif" else colors,
                                 method=Image.Quantize.MEDIANCUT)
        indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
        for frame in indexed:
            frame.info.clear() # source GIF transparency metadata does not apply to RGB copies
        path = OUTPUT / name
        indexed[0].save(path, save_all=True, append_images=indexed[1:],
                        duration=durations, loop=0, optimize=True, disposal=1)
        total += path.stat().st_size
    print(f"{colors} colors: {total:,} bytes")
    if total <= BUDGET:
        break
else:
    raise RuntimeError("GIF set exceeds the flash budget; shorten source animations")

for name in SOURCES:
    path = OUTPUT / name
    check = Image.open(path)
    assert check.size == (160, 160) and check.info.get("loop") == 0
    total_duration = 0
    for index in range(check.n_frames):
        check.seek(index)
        check.load()
        total_duration += check.info.get("duration", 0)
    assert total_duration == sum(prepared[name][1]), f"Timing changed: {name}"
    print(f"Verified {name}: {check.n_frames} frames, {path.stat().st_size:,} bytes")
