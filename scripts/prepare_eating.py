"""Trim the eating segment and fit it to the round display (Pillow + OpenCV)."""
from pathlib import Path
import shutil
import cv2
import numpy as np
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
source = Image.open(root / "data/hamster-meme-9400671.gif")
frames = []
outline = np.array([(399, 0), (466, 8), (535, 23), (602, 12),
                    (658, 17), (680, 65), (675, 104), (696, 151),
                    (713, 222), (714, 312), (751, 425), (748, 463),
                    (618, 458), (482, 420), (371, 410), (324, 368),
                    (312, 289), (302, 239), (306, 190), (322, 145),
                    (370, 112), (391, 82)])
table = np.array([(0, 399), (121, 383), (248, 371), (327, 378),
                  (425, 405), (650, 432), (889, 477), (889, 500), (0, 500)])
table_mask = np.zeros((500, 889), np.uint8)
cv2.fillPoly(table_mask, [table], 255)

for index in range(11):
    source.seek(index)
    rgb = np.array(source.convert("RGB"))
    silhouette = np.zeros((500, 889), np.uint8)
    cv2.fillPoly(silhouette, [outline], 255)
    outer = cv2.dilate(silhouette, np.ones((21, 21), np.uint8))
    inner = cv2.erode(silhouette, np.ones((65, 65), np.uint8))
    mask = np.full((500, 889), cv2.GC_BGD, np.uint8)
    mask[outer > 0] = cv2.GC_PR_BGD
    mask[silhouette > 0] = cv2.GC_PR_FGD
    mask[inner > 0] = cv2.GC_FGD
    cv2.grabCut(rgb, mask, None, np.zeros((1, 65), np.float64),
                np.zeros((1, 65), np.float64), 4, cv2.GC_INIT_WITH_MASK)
    alpha = np.where((mask == cv2.GC_FGD) | (mask == cv2.GC_PR_FGD), 255, 0).astype(np.uint8)
    alpha = np.maximum(alpha, table_mask)
    alpha = cv2.GaussianBlur(alpha, (3, 3), 0.6)
    cutout = Image.fromarray(rgb)
    black = Image.new("RGB", source.size)
    black.paste(cutout, (0, 0), Image.fromarray(alpha))
    canvas = Image.new("RGB", (580, 580))
    canvas.paste(black.crop((230, 0, 810, 500)), (0, 40))
    frames.append(canvas.resize((240, 240), Image.Resampling.LANCZOS))

strip = Image.new("RGB", (240, 240 * len(frames)))
for i, frame in enumerate(frames):
    strip.paste(frame, (0, 240 * i))
palette = strip.quantize(colors=96, method=Image.Quantize.MEDIANCUT)
indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
loop = indexed + indexed[-2:0:-1]
destination = root / "data/eating.gif"
backup = root / ".tools/eating-before-background-removal.gif"
if destination.exists() and not backup.exists():
    shutil.copyfile(destination, backup)
loop[0].save(destination, save_all=True, append_images=loop[1:], duration=70,
             loop=0, disposal=1, optimize=True)

preview_dir = root / "previews"
preview_dir.mkdir(exist_ok=True)
circle = Image.new("L", (240, 240))
ImageDraw.Draw(circle).ellipse((0, 0, 239, 239), fill=255)
previews = []
for frame in frames:
    preview = Image.new("RGB", (240, 240), (32, 32, 32))
    preview.paste(frame, (0, 0), circle)
    ImageDraw.Draw(preview).ellipse((0, 0, 239, 239), outline=(0, 220, 160))
    previews.append(preview)
preview_loop = previews + previews[-2:0:-1]
preview_loop[0].save(preview_dir / "eating-round.gif", save_all=True,
                     append_images=preview_loop[1:], duration=70, loop=0)
previews[0].resize((720, 720)).save(preview_dir / "eating-round.png")
sheet = Image.new("RGB", (720, 240))
for i, index in enumerate([0, 5, 10]):
    sheet.paste(previews[index], (240 * i, 0))
sheet.save(preview_dir / "eating-check.png")
check = Image.open(destination)
for i in range(check.n_frames):
    check.seek(i)
    check.load()
print(f"Verified {check.size}, {check.n_frames} frames, {destination.stat().st_size} bytes")
