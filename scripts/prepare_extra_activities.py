"""Prepare dance while preserving every existing device GIF byte-for-byte.
Requires Pillow. Run after prepare_preview.py if rebuilding the base set.
All source frames and timing are retained; resolution/palette adapt to headroom.
"""
from pathlib import Path
from io import BytesIO
from PIL import Image, ImageOps, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "device_data"
NAMES = ("dance.gif",)
BUDGET = 2200000
OUTPUT.mkdir(exist_ok=True)
base = sum(p.stat().st_size for p in OUTPUT.iterdir() if p.name not in NAMES)
source = {}
for name in NAMES:
    image = Image.open(ROOT / "data" / name)
    frames, durations = [], []
    for i in range(image.n_frames):
        image.seek(i)
        frames.append(image.convert("RGB"))
        durations.append(max(image.info.get("duration",40),10))
    source[name] = frames, durations

selected = None
for size in (160,144,128,120,112,96,80):
    resized = {name: [ImageOps.pad(f,(size,size),method=Image.Resampling.LANCZOS,
                   color="white").filter(ImageFilter.MedianFilter(3)) for f in frames]
               for name,(frames,_) in source.items()}
    for colors in (128,96,64,48,32):
        encoded = {}
        for name, frames in resized.items():
            strip = Image.new("RGB",(size,size*len(frames)))
            for i,frame in enumerate(frames): strip.paste(frame,(0,size*i))
            palette = strip.quantize(colors=colors,method=Image.Quantize.MEDIANCUT)
            indexed = [f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
            for frame in indexed: frame.info.clear()
            output = BytesIO()
            indexed[0].save(output,format="GIF",save_all=True,append_images=indexed[1:],
                            duration=source[name][1],loop=0,disposal=1,optimize=True)
            encoded[name] = output.getvalue()
        total = base + sum(map(len,encoded.values()))
        print(f"{size}px / {colors} colors: {total:,} total bytes")
        if total <= BUDGET:
            selected = encoded
            break
    if selected is not None: break
if selected is None:
    raise RuntimeError("New activities do not fit; existing files were left untouched")
for name, content in selected.items():
    check = Image.open(BytesIO(content))
    assert check.size == (size,size) and check.info.get("loop") == 0
    elapsed = 0
    for i in range(check.n_frames):
        check.seek(i); check.load(); elapsed += check.info.get("duration",0)
    assert elapsed == sum(source[name][1]), f"Timing changed: {name}"
    (OUTPUT/name).write_bytes(content)
    print(f"Verified {name}: {check.n_frames} frames, {len(content):,} bytes")
print(f"All seven activities: {total:,} bytes / {BUDGET:,} budget")
