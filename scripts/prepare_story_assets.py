"""Assemble ImageGen drinking sprite footage, then run prepare_preview.py for the board.
Sources: data/hami-drink-explore-sheet.png, generated from the existing eating
and idle GIF reference frames. No downloaded third-party footage is included.
Requires Pillow. Original activity GIFs remain unchanged.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sheet = Image.open(ROOT / "data/hami-drink-explore-sheet.png").convert("RGB")
w, h = sheet.size
assert w == h, "Expected a square 4 x 4 sprite sheet"
tile = w / 4
for name, first_row, timing in (("drinking.gif", 0, 60),):
    poses = []
    for row in range(first_row, first_row+2):
        for col in range(4):
            poses.append(sheet.crop(tuple(round(value*tile) for value in (col,row,col+1,row+1)))
                         .resize((240,240), Image.Resampling.LANCZOS))
    frames = []
    for i, pose in enumerate(poses):
        following = poses[(i+1) % len(poses)]
        frames += [Image.blend(pose, following, fraction/4) for fraction in range(4)]
    strip = Image.new("RGB", (240,240*len(frames)))
    for i, frame in enumerate(frames):
        strip.paste(frame,(0,240*i))
    palette = strip.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
    frames = [frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
    path = ROOT / "data" / name
    frames[0].save(path,save_all=True,append_images=frames[1:],duration=timing,
                   loop=0,disposal=1,optimize=True)
    check = Image.open(path)
    for i in range(check.n_frames):
        check.seek(i); check.load()
    print(f"{name}: {check.n_frames} frames, {path.stat().st_size:,} bytes")
