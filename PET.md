# DiGi Hami interactive pet

The pet boots into **gameplay**, showing the exercising animation. Exercising
is also the default scene after care. Random previews remain available with `r`.

## Keyboard controls

Open Serial Monitor at **115200 baud**. Type a key and send it; CR/LF line endings
are ignored. These commands come from the computer, not a USB keyboard on the ESP32.

| Key | Action |
| --- | --- |
| `g` | Gameplay; return to the wheel and resume care timers |
| `r` | Random animations every 10-20 seconds; pause care timers |
| `h` | Health screen / return to animation |
| `f` | Feed |
| `w` | Give water (thumbs-up animation until drinking.gif exists) |
| `s` | Put to sleep for 60 seconds |
| `p` | Play / exercise for 10 seconds |
| Space | Answer the oldest pending need |
| `1` / `2` / `3` / `4` / `5` | Hold idle / eating / exercising / sleeping / thumbs-up in preview mode |
| `n` | Game: request all needs immediately for testing. Random: next scene |
| `?` | Print help |

Care keys apply only in gameplay. They can be used before a request is due.
A meal, drink, or play activity lasts 10 seconds; sleep lasts 60 seconds.
A three-second thumbs-up follows care, then the wheel returns. Switching modes
ends the current activity but retains care history during this powered session.

## Button

Wire a normally open momentary button between **GPIO5 and GND**; the internal
pull-up is enabled. Tap to open/close health. Hold for 800 ms to answer the oldest
pending need in gameplay. A hold without a pending request toggles health.
The health screen also closes automatically after 30 seconds.

## Health screen

A small drawn hamster portrait appears above overall health. Four colored bars
show **food, water, energy, and fun**, each from 0 to 100. The screen also shows
time since the last meal and drink, plus meal/drink counts. Before first care,
it shows "not yet". All elapsed care times use active game time.

Scores gradually fall from 100 to 60 as the next request approaches. A pending
need falls another 10 points per overdue game hour, down to zero. Health is the
average of the four bars. The pet has no death mechanic. The initial staggered
schedule means some bars begin below 100. Care restores that need to 100.

## Requests and banners

| Need | First request after boot | Interval after care | Color |
| --- | --- | --- | --- |
| Food | 8 game hours | 8 game hours | Orange |
| Water | 4 game hours | 8 game hours | Cyan |
| Energy | 6 game hours | 12 game hours | Lavender |
| Fun | 3 game hours | 12 game hours | Lime |

This approximates three food and three staggered water requests a day, plus two
each for sleep and play. Care restarts that need's interval; exact clock times
aren't used. An unanswered need remains pending, without stacking requests.
Default wheel animation is cosmetic and does not automatically satisfy fun.

A rounded colored banner appears over the lower part of the running GIF for
five seconds, with a small hamster portrait, request text, and care key. Pending
needs rotate every 30 seconds. Banners wait during care animations. The animation
buffer is restored after each banner render so transparency/delta frames retain
their underlying pixels. The health screen takes precedence over alerts.

**Game time and care decay pause in random/manual preview mode.** They continue
on the health screen while in gameplay. **Care history and timers are saved in internal flash and restored after
restart.** No Wi-Fi or real-time clock is required.

Sound remains disabled (`PET_BUZZER_PIN = -1`). A suitable passive piezo or driver
circuit can be added later; the firmware has an optional request chirp.

## Saving and power loss

The pet uses Arduino Preferences (NVS), separate from the animation filesystem.
It saves active game time, request deadlines, pending needs, last-care times,
and all four care counts. Health is recalculated from those saved values.

Care actions and new pending requests save immediately. Entering random/manual
preview mode also saves a checkpoint. During normal gameplay, elapsed time is
saved every five minutes, avoiding continuous flash writes. An abrupt power
loss may roll back up to five minutes of elapsed gameplay since the last save;
completed care is saved immediately when the flash write succeeds.

Offline time does not count. On boot the pet returns to gameplay on the wheel,
with saved care history; it does not resume a partially played care animation.
The previous display/mode selection is not saved. Serial Monitor reports restore
or save failures. First boot without a saved state starts a new pet. Normal
firmware/filesystem uploads keep NVS with the current partition layout; a full
flash erase or changing the NVS partition can erase history.

To verify on the board: feed once with `f`, open health with `h`, note Meals,
then unplug and reconnect. Meals and Fed should restore, and the unpowered
interval should not be added. Send `n`, restart again, and verify pending
requests return. This power-cycle check has not been performed automatically.

## Quick checks after uploading

1. Confirm the pet starts on the wheel. Send `h` and inspect health, then `h` again.
2. Send `n` to trigger all needs without waiting hours. Verify a colored banner
   appears over the moving GIF, then clears without leaving colored pixels.
3. Send `f`. Confirm eating, then thumbs-up, then exercising. Send `h`: food should
   read 100, Meals should increase, and Fed should show time since that action.
4. Send `w`, `s`, and `p` to exercise the other care actions. Water uses thumbs-up.
5. Send `r`: random scenes should run with no care banners. Send `g` to return to
   the wheel; previously pending needs and care history should still be present.
6. When a button is fitted, test tap health/back and hold to answer a pending need.

The firmware was compiled successfully; these physical display/button checks
still need to be performed on the board. Nothing was automatically flashed.

### Device files and storage

Originals remain in `data/`. The board uses only the five prepared GIFs in
`device_data/`, as configured by `data_dir` in `platformio.ini`. This excludes
the large source meme and duplicate GIFs. Each prepared file uses the same
activity filename as its source.

Run `python scripts/prepare_preview.py` after replacing source animations
(requires Pillow). It fits each scene into 160×160 with white padding as needed,
retains all source frames and their timing, and lightly filters grain for better
compression. It chooses the highest palette size that fits the 2.2 MB asset
budget (currently 64 colors). Firmware scales the scene to fill 240×240 in a
115,200-byte RGB565 frame buffer and sends it in one SPI transaction per frame.
This trades fine spatial detail for smoother motion and more colors than the
previous 10 FPS / 32-color files. Prepared files are already included, so normal
builds do not require Pillow. A source animation originally recorded at 10 FPS
still plays at 10 FPS; extra motion is not synthesized.

Idle is the exception: it plays forward and backward with a lighter honey-brown
coat. It uses approximately 20 FPS and 32 colors to fit the longer loop; other
scenes keep their source timing and 64 colors. Original GIFs remain unchanged.

`partitions.csv` reserves 1 MB for firmware and about 3 MB for SPIFFS on the
4 MB board. This layout supports USB flashing rather than two-slot OTA updates.
**The first upload after this change needs both firmware and filesystem** because
the filesystem offset changed. The existing Upload hook uploads both; you can
also run Upload and Upload Filesystem Image separately. Thereafter filesystem
upload uses `device_data/`, not `data/`.

