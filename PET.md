# DiGi Hami interactive pet

The pet boots into **autopilot**, replacing random playback. Health and care
history keep running in auto mode. Each chapter displays a five-second alert
before it starts, with the previous animation continuing behind the banner.
Activity notices show only the chapter title: no countdown, Hami portrait, or speech.

## Repeating story

| Chapter | Duration |
| --- | --- |
| Wake up (idle.gif) | 30 seconds |
| Eat | 7 minutes |
| Explore (idle.gif) | 30 seconds |
| Wheel exercise | 40 minutes |
| Health check screen | 30 seconds |
| Explore (idle.gif) | 30 seconds |
| Dance (dance.gif) | 2 minutes |
| Drink (new blue-bowl GIF) | 30 seconds |
| Sleep | 30 minutes |
| Wake and explore (idle.gif) | 30 seconds |
| Health check screen | 30 seconds |

A single-play happy thumbs-up follows meals, drinks, sleep, dance, exercise, and each health
check, in both auto and manual game modes. All auto idle/explore chapters last
30 seconds. The story repeats. Durations and chapter titles are in `story` in `src/pet.h`.
Meals, drinks, exercise, and sleep restore their associated need and update saved
care history when that chapter starts. Health checks show overall health, food,
water, energy, fun, and meal/drink history. The 30 seconds refers to how long the
health screen is displayed, after exercise and after waking/exploring.

## Controls

Open Serial Monitor at **115200 baud**. CR/LF line endings are ignored.

| Key | Action |
| --- | --- |
| `a` or `r` | Start autopilot from waking up |
| `g` or `G` | Switch to manual gameplay; idle/explore while waiting for care |
| `h` | Health screen / return; closes after 30 seconds |
| `n` | Auto: finish activity with its happy ending, then next chapter; manual game: request all needs |
| `f`, `w`, `s`, `p` | Manual feed, water, sleep, play; exits autopilot |
| Space | Answer oldest request; exits autopilot |
| `1` / `2` / `3` / `4` / `5` / `6` | Hold idle / eating / exercise / sleep / thumbs-up / dance preview |
| `?` | Print help |

Manual care uses the same durations: seven-minute meals, 30-second drinks,
30-minute naps, and 40-minute exercise. A single-play thumbs-up follows care,
then idle returns. Preview mode pauses health timers; autopilot and manual game
keep them running. Switching modes ends the current chapter.

Wire a normally open button between **GPIO5 and GND**. Tap for health/back.
Hold 800 ms to answer the oldest pending need (switching to manual gameplay);
a hold without a pending need toggles health.

## Messages from Hami

Autopilot is always the startup default, including after a restart. Press `g`
(or `G`) in Serial Monitor to enter game mode; `a` returns to autopilot.

Hami waits about four seconds after a new activity starts before speaking,
including care thank-yous. Messages then last six seconds and recur about every
15 seconds during activities
in both modes. There are 54 short messages: six for each activity, plus twelve
kind classroom messages. Examples include "Zzz...", "Boy, that's good!",
"Slurp, slurp!", "Faster, faster!", "You belong here!", and "Mistakes are okay".
Sleep uses only cozy sleep messages. Other activities mix in encouragement,
curiosity, kindness, and taking turns. Each bank cycles through its own messages.
Care actions also show a thank-you. Activity notices have no Hami speech.
Health screens and the happy thumbs-up GIF stay unobstructed. Thumbs-up plays
once, then moves directly into the next activity when the GIF finishes, without
a next-activity notice or five-second pause. Its last frame gets its normal
display time.
The manual thumbs-up preview holds that frame until another scene is selected.

Pressing `n` during a care activity or health check plays its full
thumbs-up once before advancing directly to the next activity. Repeated `n` presses during the happy ending do not
skip it. Idle chapters move straight to the next notice. Happy endings also run
when activities finish naturally.

## Health and saving

Scores fall from 100 to 60 as a request approaches, then another 10 points per
overdue active hour, down to zero. Overall health averages the four needs.
The pet has no death mechanic. Food/water intervals are eight active hours;
energy/fun intervals are twelve. Initial requests are staggered at eight,
four, six, and three hours respectively. Autopilot's regular care keeps needs
replenished. Manual requests show rotating five-second banners every 30 seconds.

Care and pending requests save immediately in Preferences/NVS; elapsed active
time saves every five minutes. Offline time pauses. Restart restores health and
care history and starts the waking-up chapter; chapter progress is not saved.
A power loss can roll elapsed time back by up to five minutes. Preview selection
is not saved. Sound is disabled (`PET_BUZZER_PIN = -1`).

## Board checks

1. Boot: verify idle and a five-second waking-up banner.
2. Send `n`: verify the next alert, then eating after five seconds.
3. While eating, press `n`: verify one unobstructed thumbs-up playback, then
   the next activity directly without a notice or five-second wait. Repeated `n` must not skip the happy GIF. Repeat
   after exercise, health, drinking, and sleep. Check chatter during idle/drinking
   (15 seconds), and during longer activities. Test both `a` and `g` modes.
4. Verify health displays for 30 seconds, with saved care counts increasing.
5. Test `h`, manual care, preview holds, `a` to resume, and the optional button.
6. Power-cycle after a meal; verify its saved count and paused offline aging.

Physical display/button checks require the board; firmware is not automatically
flashed by this change.

### Device files and storage

Originals remain in `data/`. The board uses only the seven prepared GIFs in
`device_data/`, as configured by `data_dir` in `platformio.ini`. This excludes
the large source meme and duplicate GIFs. Each prepared file uses the same
activity filename as its source.

Run `python scripts/prepare_preview.py` after replacing source animations
(requires Pillow). It fits each scene into 160×160 with white padding as needed,
retains all source frames and their timing, and lightly filters grain for better
compression. It chooses the highest palette size that fits the 2.2 MB asset
budget (32 colors for the other prepared scenes; restored idle uses 32 colors). Firmware scales the scene to fill 240×240 in a
115,200-byte RGB565 frame buffer and sends it in one SPI transaction per frame.
This trades fine spatial detail for smoother motion and more colors than the
previous 10 FPS / 32-color files. Prepared files are already included, so normal
builds do not require Pillow. A source animation originally recorded at 10 FPS
still plays at 10 FPS; extra motion is not synthesized.

Idle is the exception: it plays forward and backward with a lighter honey-brown
coat. It uses approximately 20 FPS and 32 colors to fit the longer loop; other
scenes keep their source timing. Idle uses its original prepared forward/backward loop. Original GIFs remain unchanged.

`partitions.csv` reserves 1 MB for firmware and about 3 MB for SPIFFS on the
4 MB board. This layout supports USB flashing rather than two-slot OTA updates.
**The first upload after this change needs both firmware and filesystem** because
the filesystem offset changed. The existing Upload hook uploads both; you can
also run Upload and Upload Filesystem Image separately. Thereafter filesystem
upload uses `device_data/`, not `data/`.


### New drinking and explore footage

`data/drinking.gif` is a distinct blue-bowl sipping loop, generated using the
existing eating and idle GIF frames as appearance references. The explore/idle animation has been restored to the previous prepared loop;
generated explore footage is no longer included. Drinking uses generated poses
with blended in-between frames, not downloaded video.

The built-in ImageGen tool made `data/hami-drink-explore-sheet.png`. Prompt:
"4 by 4 photorealistic sprite sheet; top eight frames: golden Syrian hamster
sipping from a blue water bowl on the mint tabletop; bottom eight: sniffing and
looking around the wooden hideout in bedding; preserve reference appearance,
locked camera, consistent lighting, no text."

Run `python scripts/prepare_story_assets.py` to reassemble the drinking source
GIF, then `python scripts/prepare_preview.py` to prepare all six device GIFs.
The device preparation checks frame decoding, dimensions, loop timing, and the
2.2 MB storage budget. Upload both firmware and filesystem for these changes.

### Dance

Autopilot includes two minutes of dance between the explore break and water.
Dance restores fun and ends with a single thumbs-up, then continues directly.
It has six classroom-friendly messages; the first waits four seconds as in the
other activities. Preview with `6`; use `a` to return to autopilot.

The separate play activity and its device GIF have been removed. The original
`data/play.gif` remains available as a source; it is not flashed. Manual `p`
continues to provide the original wheel exercise care action.

Run `python scripts/prepare_extra_activities.py` to prepare dance without
modifying the six other device GIFs. If rebuilding everything, run
`prepare_preview.py` first, then `prepare_extra_activities.py`. Dance keeps all
52 frames and their original timing, at 120x120 and 32 colors, improved from
96x96. Firmware scales it to the display. All seven files total 2,191,469 bytes,
below the 2,200,000-byte asset budget. Upload both firmware and filesystem.
