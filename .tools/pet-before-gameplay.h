#pragma once

// All intervals are powered-on time. No Wi-Fi or wall clock is required.
constexpr int PET_BUTTON_PIN = 5; // momentary button between GPIO5 and GND
constexpr int PET_BUZZER_PIN = -1; // optional passive piezo; -1 disables sound
constexpr uint32_t PET_HOUR = 60UL * 60UL * 1000UL;
constexpr bool PET_PREVIEW_MODE = true; // false restores the care timers/button mode
enum Activity { IDLE, EATING, DRINKING, SLEEPING, EXERCISING, THUMBS_UP };
const char *activityNames[] = {"Idle", "Eating", "Drinking", "Sleeping", "Exercising", "Thumbs up"};
const char *animationPaths[] = {"/idle.gif", "/eating.gif", "/drinking.gif", "/sleeping.gif", "/exercising.gif", "/thumbs-up.gif"};
const Activity previewActivities[] = {IDLE, EATING, EXERCISING, SLEEPING, THUMBS_UP};
bool randomPlayback = true;
uint32_t previewSwitchAt = 0;
struct Need {
  const char *label;
  const char *message;
  uint32_t interval;
  uint32_t next;
  Activity activity;
  bool pending;
  uint32_t since;
  unsigned count;
};
Need needs[] = {
  {"Food", "FEED ME", 8 * PET_HOUR, 8 * PET_HOUR, EATING, false, 0, 0},
  {"Water", "THIRSTY", 8 * PET_HOUR, 4 * PET_HOUR, DRINKING, false, 0, 0},
  {"Energy", "SLEEPY", 12 * PET_HOUR, 6 * PET_HOUR, SLEEPING, false, 0, 0},
  {"Fun", "PLAY WITH ME", 12 * PET_HOUR, 3 * PET_HOUR, EXERCISING, false, 0, 0}
};
Activity activity = IDLE;
uint32_t activityStarted = 0, activityDuration = 0, nextIdle = 0;
uint32_t frameDue = 0, nextNotice = 0, screenStarted = 0, lastStats = 0;
bool statsScreen = false, noticeScreen = false, filesystemReady = false;
String openAnimation;

bool reached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

void centered(const char *text, int y, int size, uint16_t color = 0xFFFF) {
  tft.setTextSize(size);
  tft.setTextColor(color);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((DISPLAY_WIDTH - (int)w) / 2, y);
  tft.print(text);
}

void selectAnimation(Activity requested) {
  const char *path = animationPaths[requested];
  if (!filesystemReady) return;
  if (!SPIFFS.exists(path)) path = "/animation.gif";
  if (gifReady && openAnimation == path) {
    gif.reset();
  } else {
    if (gifReady) gif.close();
    gifReady = gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
    if (gifReady && (gif.getCanvasWidth() > DISPLAY_WIDTH || gif.getCanvasHeight() > DISPLAY_HEIGHT)) {
      Serial.printf("GIF too large: %s (maximum 240x240)\n", path);
      gif.close();
      gifReady = false;
    }
    openAnimation = gifReady ? path : "";
  }
  frameDue = millis();
}

void showPet() {
  statsScreen = noticeScreen = false;
  memset(frameBuffer, 0, sizeof(frameBuffer));
  tft.fillScreen(0);
  selectAnimation(activity);
  if (!gifReady) {
    centered(activityNames[activity], 96, 2);
    centered("Add GIF to data/", 126, 1);
  }
}

int oldestNeed() {
  int selected = -1;
  uint32_t now = millis();
  for (int i = 0; i < 4; ++i)
    if (needs[i].pending && (selected < 0 || now - needs[i].since > now - needs[selected].since)) selected = i;
  return selected;
}

int needScore(const Need &need) {
  if (!need.pending) return 100;
  uint32_t overdueHours = (millis() - need.since) / PET_HOUR;
  return 70 - (int)min(overdueHours, (uint32_t)7) * 10;
}

void showStats() {
  statsScreen = true;
  noticeScreen = false;
  screenStarted = lastStats = millis();
  tft.fillScreen(0);
  centered("HAMI STATS", 26, 2);
  int health = 0;
  char text[40];
  for (int i = 0; i < 4; ++i) {
    int score = needScore(needs[i]);
    health += score;
    snprintf(text, sizeof(text), "%s %3d  done %u", needs[i].label, score, needs[i].count);
    centered(text, 64 + i * 24, 1, needs[i].pending ? 0xFFE0 : 0x07E0);
  }
  snprintf(text, sizeof(text), "Health: %d/100", health / 4);
  centered(text, 170, 2);
  centered("Tap: back  Hold: stats", 202, 1);
}

void startActivity(Activity next, uint32_t duration) {
  activity = next;
  activityStarted = millis();
  activityDuration = duration;
  Serial.printf("Activity: %s\n", activityNames[next]);
  showPet();
}

void careFor(int index) {
  // Only requested care changes the stats; repeated presses cannot farm points.
  if (index < 0 || !needs[index].pending) return;
  needs[index].pending = false;
  ++needs[index].count;
  startActivity(needs[index].activity, needs[index].activity == SLEEPING ? 60000 : 10000);
  nextNotice = millis() + 15000;
}

void shortPress() {
  if (statsScreen) { showPet(); return; }
  int index = oldestNeed();
  if (index >= 0) careFor(index);
  else showStats();
}

void pollButton(uint32_t now) {
  static bool previousRaw = HIGH, stable = HIGH, held = false;
  static uint32_t changed = 0, pressed = 0;
  bool raw = digitalRead(PET_BUTTON_PIN);
  if (raw != previousRaw) { previousRaw = raw; changed = now; }
  if (raw != stable && now - changed >= 30) {
    stable = raw;
    if (!stable) { pressed = now; held = false; }
    else if (!held) shortPress();
  }
  if (!stable && !held && now - pressed >= 800) {
    held = true;
    if (statsScreen) showPet(); else showStats();
  }
}

void petSetup(bool mounted) {
  filesystemReady = mounted;
  pinMode(PET_BUTTON_PIN, INPUT_PULLUP);
  nextIdle = millis() + 30000;
  nextNotice = millis();
  showPet();
  if (PET_PREVIEW_MODE) {
    previewSwitchAt = millis() + random(10000, 20001);
    Serial.println("Preview: 1/i=idle 2/e=eating 3/x=exercise 4/s=sleep 5/t=thumbs-up");
    Serial.println("r=random playback; n=next random; ?=help. A selection holds until r or n.");
    return;
  }
  Serial.println("Pet ready. Serial: f=food w=water s=sleep p=play h=stats n=request all");
}

void playAnimationFrame() {
  if (!gifReady || !reached(millis(), frameDue)) return;
  int frameDelay = 0;
  uint32_t started = millis();
  frameDrawn = false;
  int result = gif.playFrame(false, &frameDelay);
  if (result < 0) {
    Serial.println("GIF decode failed; stopping this animation until the next activity.");
    gif.close();
    gifReady = false;
    centered("GIF error", 110, 2);
  } else {
    if (frameDrawn) {
      tft.startWrite();
      tft.setAddrWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
      tft.writePixels(frameBuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT);
      tft.endWrite();
    }
    if (result == 0) gif.reset();
    frameDue = started + (uint32_t)max(frameDelay, 10);
  }
}

void previewSelect(Activity next) {
  activity = next;
  previewSwitchAt = millis() + random(10000, 20001);
  Serial.printf("Preview: %s (%s)\n", activityNames[next], randomPlayback ? "random" : "manual hold");
  showPet();
}

void previewRandom() {
  // Choose among the other four animations so adjacent scenes never repeat.
  Activity choices[5];
  int count = 0;
  for (Activity candidate : previewActivities)
    if (candidate != activity) choices[count++] = candidate;
  previewSelect(choices[random(count)]);
}

void previewLoop() {
  while (Serial.available()) {
    char command = Serial.read();
    int selected = -1;
    if (command >= '1' && command <= '5') selected = command - '1';
    if (command == 'i') selected = 0;
    if (command == 'e') selected = 1;
    if (command == 'x') selected = 2;
    if (command == 's') selected = 3;
    if (command == 't') selected = 4;
    if (selected >= 0) {
      randomPlayback = false;
      previewSelect(previewActivities[selected]);
    } else if (command == 'r' || command == 'n') {
      randomPlayback = true;
      previewRandom();
    } else if (command == '?') {
      Serial.println("1/i idle | 2/e eating | 3/x exercise | 4/s sleep | 5/t thumbs-up | r random | n next");
    }
    // Ignore CR/LF so either Serial Monitor line-ending setting works.
  }
  if (randomPlayback && reached(millis(), previewSwitchAt)) previewRandom();
  playAnimationFrame();
  delay(1);
}

void petLoop() {
  if (PET_PREVIEW_MODE) { previewLoop(); return; }
  uint32_t now = millis();
  pollButton(now);
  for (int i = 0; i < 4; ++i) {
    if (reached(now, needs[i].next)) {
      // Coalesce missed requests rather than building up a queue.
      needs[i].next = now + needs[i].interval;
      if (!needs[i].pending) {
        needs[i].pending = true;
        needs[i].since = now;
        Serial.println(needs[i].message);
        if (PET_BUZZER_PIN >= 0) tone(PET_BUZZER_PIN, 2400, 120);
      }
    }
  }
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'f') careFor(0);
    if (command == 'w') careFor(1);
    if (command == 's') careFor(2);
    if (command == 'p') careFor(3);
    if (command == 'h') { if (statsScreen) showPet(); else showStats(); }
    if (command == 'n') {
      for (auto &need : needs) if (!need.pending) { need.pending = true; need.since = now; }
      nextNotice = now;
    }
  }
  // Input handlers may have started an activity after the earlier time sample.
  now = millis();
  if (activity != IDLE && now - activityStarted >= activityDuration) {
    activity = IDLE;
    nextIdle = now + random(30000, 90001);
    if (!statsScreen && !noticeScreen) showPet();
  }
  if (statsScreen) {
    if (now - screenStarted >= 15000) showPet();
    else if (now - lastStats >= 1000) {
      uint32_t originalStart = screenStarted;
      showStats();
      screenStarted = originalStart;
    }
    return;
  }
  if (noticeScreen) {
    if (now - screenStarted >= 2000) showPet();
    return;
  }
  int pending = oldestNeed();
  if (pending >= 0 && activity == IDLE && reached(now, nextNotice)) {
    noticeScreen = true;
    screenStarted = now;
    nextNotice = now + 30000;
    tft.fillScreen(0);
    centered(needs[pending].message, 96, 2, 0xFFE0);
    centered("Press to help!", 132, 1);
    return;
  }
  if (pending < 0 && activity == IDLE && reached(now, nextIdle)) {
    // Ambient wheel exercise is cosmetic; it does not answer a play request.
    startActivity(EXERCISING, random(5000, 12001));
  }
  playAnimationFrame();
  delay(1);
}
