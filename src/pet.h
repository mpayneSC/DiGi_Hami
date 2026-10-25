#pragma once
#include <Preferences.h>

constexpr int PET_BUTTON_PIN = 5; // Normally open button between GPIO5 and GND.
constexpr int PET_BUZZER_PIN = -1;
constexpr uint64_t PET_HOUR = 3600000ULL;
enum Activity { IDLE, EATING, DRINKING, SLEEPING, EXERCISING, THUMBS_UP };
const char *activityNames[] = {"Idle", "Eating", "Drinking", "Sleeping", "Exercising", "Thumbs up"};
const char *animationPaths[] = {"/idle.gif", "/eating.gif", "/drinking.gif", "/sleeping.gif", "/exercising.gif", "/thumbs-up.gif"};
const Activity previewActivities[] = {IDLE, EATING, EXERCISING, SLEEPING, THUMBS_UP};
struct Need {
  const char *label, *alert, *hint;
  uint64_t interval, next;
  Activity activity;
  uint16_t color;
  bool pending;
  uint64_t since, lastCare;
  unsigned count;
};
Need needs[] = {
  {"Food", "I'M HUNGRY!", "F: feed / hold", 8*PET_HOUR, 8*PET_HOUR, EATING, 0xFD20, false, 0, 0, 0},
  {"Water", "I'M THIRSTY!", "W: water / hold", 8*PET_HOUR, 4*PET_HOUR, DRINKING, 0x05FF, false, 0, 0, 0},
  {"Energy", "I'M SLEEPY!", "S: sleep / hold", 12*PET_HOUR, 6*PET_HOUR, SLEEPING, 0xB59F, false, 0, 0, 0},
  {"Fun", "LET'S PLAY!", "P: play / hold", 12*PET_HOUR, 3*PET_HOUR, EXERCISING, 0x87E0, false, 0, 0, 0}
};
bool gameMode = true, randomPlayback = true;
bool statsScreen = false, filesystemReady = false;
Activity activity = EXERCISING;
uint64_t gameTime = 0;
uint32_t lastTick = 0, frameDue = 0, activityStarted = 0, activityDuration = 0;
uint32_t previewSwitchAt = 0, statsStarted = 0, lastStats = 0;
uint32_t nextNotice = 0, noticeStarted = 0;
int noticeNeed = -1, noticeCursor = 0;
String openAnimation;
constexpr uint16_t UI_BG = 0x1082, UI_TEXT = 0xFFFF;

// Store values only, never pointers or millis() deadlines. Game time advances
// only while powered and in gameplay, so offline time cannot age the pet.
struct SavedNeed {
  uint64_t next, since, lastCare;
  uint32_t count, pending;
};
struct SavedPet {
  uint32_t magic, version;
  uint64_t gameTime;
  SavedNeed needs[4];
};
constexpr uint32_t SAVE_MAGIC = 0x48414D49, SAVE_VERSION = 1;
constexpr uint32_t AUTOSAVE_MS = 5UL * 60UL * 1000UL;
SavedPet lastSavedState = {};
bool haveSavedState = false;
uint32_t lastSaveAttempt = 0;

bool savePet() {
  SavedPet state = {};
  state.magic = SAVE_MAGIC; state.version = SAVE_VERSION; state.gameTime = gameTime;
  for (int i = 0; i < 4; ++i) {
    state.needs[i] = {needs[i].next, needs[i].since, needs[i].lastCare,
                      (uint32_t)needs[i].count, needs[i].pending ? 1U : 0U};
  }
  lastSaveAttempt = millis();
  if (haveSavedState && memcmp(&state,&lastSavedState,sizeof(state)) == 0) return true;
  Preferences storage;
  if (!storage.begin("digi-hami",false)) {
    Serial.println("Pet save failed: could not open flash storage."); return false;
  }
  // One NVS blob commits the whole snapshot together, including care timestamps.
  bool saved = storage.putBytes("state",&state,sizeof(state)) == sizeof(state);
  storage.end();
  if (saved) { lastSavedState = state; haveSavedState = true; }
  else Serial.println("Pet save failed; changes remain in RAM until a later save.");
  return saved;
}

void loadPet() {
  Preferences storage;
  if (!storage.begin("digi-hami",false)) {
    Serial.println("Pet restore unavailable; starting with default stats."); return;
  }
  size_t size = storage.getBytesLength("state");
  SavedPet state = {};
  bool valid = size == sizeof(state) && storage.getBytes("state",&state,sizeof(state)) == sizeof(state);
  storage.end();
  if (size == 0) { Serial.println("New pet: no saved care history yet."); savePet(); return; }
  valid = valid && state.magic == SAVE_MAGIC && state.version == SAVE_VERSION;
  for (int i = 0; valid && i < 4; ++i) {
    const SavedNeed &n = state.needs[i];
    valid = n.pending <= 1 && n.since <= state.gameTime && n.lastCare <= state.gameTime;
    if (n.next > state.gameTime) valid = valid && n.next-state.gameTime <= needs[i].interval;
  }
  if (!valid) {
    Serial.println("Saved pet has an unsupported or invalid format; using default stats."); return;
  }
  gameTime = state.gameTime;
  for (int i = 0; i < 4; ++i) {
    needs[i].next = state.needs[i].next; needs[i].since = state.needs[i].since;
    needs[i].lastCare = state.needs[i].lastCare; needs[i].count = state.needs[i].count;
    needs[i].pending = state.needs[i].pending != 0;
  }
  lastSavedState = state; haveSavedState = true; lastSaveAttempt = millis();
  Serial.println("Pet restored. Offline time paused; returning to the exercise wheel.");
}

bool reached(uint32_t now, uint32_t deadline) { return (int32_t)(now - deadline) >= 0; }

// Draw into the existing animation buffer without allocating a second screen.
class PetCanvas : public Adafruit_GFX {
public:
  PetCanvas() : Adafruit_GFX(DISPLAY_WIDTH, DISPLAY_HEIGHT) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && y >= 0 && x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT)
      frameBuffer[y * DISPLAY_WIDTH + x] = color;
  }
} ui;

void presentFrame() {
  tft.startWrite();
  tft.setAddrWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  tft.writePixels(frameBuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT);
  tft.endWrite();
}

void centered(const char *text, int y, int size, uint16_t color = UI_TEXT) {
  ui.setTextSize(size); ui.setTextColor(color);
  int16_t x1, y1; uint16_t w, h;
  ui.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  ui.setCursor((DISPLAY_WIDTH - (int)w) / 2, y); ui.print(text);
}

void hamsterPortrait(int cx, int cy, int radius) {
  uint16_t fur = 0xDCCD, dark = 0xA2E7, pink = 0xFBAE;
  ui.fillCircle(cx-radius*2/3, cy-radius*2/3, radius/2, dark);
  ui.fillCircle(cx+radius*2/3, cy-radius*2/3, radius/2, dark);
  ui.fillCircle(cx-radius*2/3, cy-radius*2/3, radius/3, pink);
  ui.fillCircle(cx+radius*2/3, cy-radius*2/3, radius/3, pink);
  ui.fillCircle(cx, cy, radius, fur);
  ui.fillCircle(cx-radius/3, cy+radius/3, radius/2, 0xFF56);
  ui.fillCircle(cx+radius/3, cy+radius/3, radius/2, 0xFF56);
  ui.fillCircle(cx-radius/3, cy-radius/5, max(1, radius/8), 0);
  ui.fillCircle(cx+radius/3, cy-radius/5, max(1, radius/8), 0);
  ui.fillTriangle(cx-2,cy+radius/5,cx+2,cy+radius/5,cx,cy+radius/5+3,pink);
  ui.drawFastVLine(cx,cy+radius/5+3,max(1,radius/5),dark);
}

void selectAnimation(Activity requested) {
  const char *path = animationPaths[requested];
  if (!filesystemReady) return;
  // A thumbs-up acknowledges water until a drinking animation is available.
  if (requested == DRINKING && !SPIFFS.exists(path)) path = "/thumbs-up.gif";
  if (!SPIFFS.exists(path)) path = "/exercising.gif";
  if (!SPIFFS.exists(path)) path = "/animation.gif";
  if (gifReady && openAnimation == path) gif.reset();
  else {
    if (gifReady) gif.close();
    gifReady = gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
    if (gifReady && (gif.getCanvasWidth() > DISPLAY_WIDTH || gif.getCanvasHeight() > DISPLAY_HEIGHT)) {
      gif.close(); gifReady = false;
    }
    openAnimation = gifReady ? path : "";
  }
  frameDue = millis();
}

void showPet() {
  statsScreen = false;
  memset(frameBuffer, 0, sizeof(frameBuffer));
  selectAnimation(activity);
  if (!gifReady) {
    hamsterPortrait(120,85,28);
    centered(activityNames[activity],130,2);
    centered("Upload device GIFs",158,1); presentFrame();
  }
}

int oldestNeed() {
  int selected = -1;
  for (int i = 0; i < 4; ++i)
    if (needs[i].pending && (selected < 0 || needs[i].since < needs[selected].since)) selected = i;
  return selected;
}

int needScore(const Need &need) {
  if (need.pending) return 60 - (int)min((gameTime-need.since)/PET_HOUR,(uint64_t)6)*10;
  uint64_t remaining = need.next > gameTime ? need.next-gameTime : 0;
  return 60 + (int)(40*min(remaining,need.interval)/need.interval);
}

void elapsedText(char *text, size_t size, const char *label, uint64_t elapsed, bool cared) {
  if (!cared) snprintf(text,size,"%s: not yet",label);
  else snprintf(text,size,"%s: %luh %02lum ago",label,
    (unsigned long)(elapsed/PET_HOUR),(unsigned long)((elapsed/60000)%60));
}

void drawStats() {
  ui.fillScreen(UI_BG); centered("HAMI HEALTH",23,2);
  hamsterPortrait(120,67,23);
  int health = 0;
  for (auto &need : needs) health += needScore(need);
  char text[48];
  snprintf(text,sizeof(text),"Health %d/100",health/4); centered(text,98,1,0x87E0);
  for (int i = 0; i < 4; ++i) {
    int y = 117+i*14, score = needScore(needs[i]);
    ui.setTextSize(1); ui.setTextColor(UI_TEXT); ui.setCursor(45,y); ui.print(needs[i].label);
    ui.fillRoundRect(89,y,79,7,3,0x3186);
    ui.fillRoundRect(89,y,max(1,79*score/100),7,3,needs[i].color);
    ui.setCursor(176,y); ui.print(score);
  }
  elapsedText(text,sizeof(text),"Fed",gameTime-needs[0].lastCare,needs[0].count>0); centered(text,177,1);
  elapsedText(text,sizeof(text),"Water",gameTime-needs[1].lastCare,needs[1].count>0); centered(text,189,1);
  snprintf(text,sizeof(text),"Meals %u | Drinks %u",needs[0].count,needs[1].count); centered(text,201,1);
  centered(gameMode ? "GAME | tap: back" : "RANDOM | game paused",214,1,0xBDF7);
  presentFrame(); lastStats = millis();
}

void showStats() { statsScreen = true; statsStarted = millis(); drawStats(); }
void toggleStats() { if (statsScreen) showPet(); else showStats(); }

void startActivity(Activity next, uint32_t duration) {
  activity = next; activityStarted = millis(); activityDuration = duration;
  showPet(); Serial.printf("Activity: %s\n",activityNames[next]);
}

void careFor(int index) {
  if (!gameMode || index < 0 || index > 3) return;
  Need &need = needs[index];
  need.pending = false; need.lastCare = gameTime; ++need.count;
  need.next = gameTime+need.interval;
  savePet(); // Commit care immediately, before starting its animation.
  noticeNeed = -1; nextNotice = millis()+15000;
  startActivity(need.activity,index == 2 ? 60000 : 10000);
  Serial.printf("%s cared for; total %u\n",need.label,need.count);
}

void pollButton(uint32_t now) {
  static bool previousRaw = HIGH, stable = HIGH, held = false;
  static uint32_t changed = 0, pressed = 0;
  bool raw = digitalRead(PET_BUTTON_PIN);
  if (raw != previousRaw) { previousRaw = raw; changed = now; }
  if (raw != stable && now-changed >= 30) {
    stable = raw;
    if (!stable) { pressed = now; held = false; }
    else if (!held) toggleStats();
  }
  if (!stable && !held && now-pressed >= 800) {
    held = true;
    if (gameMode && oldestNeed() >= 0) careFor(oldestNeed());
    else toggleStats();
  }
}

void overlayAlert() {
  if (!gameMode || noticeNeed < 0) { presentFrame(); return; }
  // Save and restore only the banner rows, preserving GIF delta-frame pixels.
  constexpr int TOP = 167, ROWS = 45;
  static uint16_t saved[DISPLAY_WIDTH*ROWS];
  memcpy(saved,frameBuffer+TOP*DISPLAY_WIDTH,sizeof(saved));
  Need &need = needs[noticeNeed];
  ui.fillRoundRect(39,169,162,40,12,need.color);
  hamsterPortrait(58,189,11);
  ui.setTextColor(0); ui.setTextSize(1);
  ui.setCursor(78,178); ui.print(need.alert);
  ui.setCursor(78,194); ui.print(need.hint);
  presentFrame();
  memcpy(frameBuffer+TOP*DISPLAY_WIDTH,saved,sizeof(saved));
}

void playAnimationFrame(bool force = false) {
  if (!gifReady || !reached(millis(),frameDue)) { if (force) overlayAlert(); return; }
  int frameDelay = 0;
  uint32_t started = millis(); frameDrawn = false;
  int result = gif.playFrame(false,&frameDelay);
  if (result < 0) {
    gif.close(); gifReady = false;
    Serial.println("GIF decode failed; try selecting another scene.");
    centered("GIF error",110,2); overlayAlert();
  } else {
    if (frameDrawn || force) overlayAlert();
    if (result == 0) gif.reset();
    frameDue = started+(uint32_t)max(frameDelay,10);
  }
}

void previewSelect(Activity next) {
  activityDuration = 0; noticeNeed = -1; activity = next;
  previewSwitchAt = millis()+random(10000,20001); showPet();
  Serial.printf("Random mode: %s (%s)\n",activityNames[next],randomPlayback ? "auto" : "hold");
}

void previewRandom() {
  Activity choices[5]; int count = 0;
  for (Activity candidate : previewActivities) if (candidate != activity) choices[count++] = candidate;
  previewSelect(choices[random(count)]);
}

void setMode(bool game) {
  savePet(); // Checkpoint elapsed gameplay before entering random mode.
  gameMode = game; noticeNeed = -1; nextNotice = millis();
  if (game) startActivity(EXERCISING,0);
  else { randomPlayback = true; previewRandom(); }
  Serial.println(game ? "GAME: care timers running" : "RANDOM: care timers paused");
}

void printHelp() {
  Serial.println("g game | r random | h health | f feed | w water | s sleep | p play");
  Serial.println("1 idle | 2 eating | 3 exercise | 4 sleeping | 5 thumbs-up (manual preview)");
  Serial.println("n: request all in game / next scene in random | space: answer oldest | ?: help");
  Serial.println("Button GPIO5-GND: tap health/back; hold 800 ms to answer oldest request");
}

void handleKeyboard() {
  while (Serial.available()) {
    char key = Serial.read();
    if (key == 'g') setMode(true);
    else if (key == 'r') setMode(false);
    else if (key == 'h') toggleStats();
    else if (key == '?') printHelp();
    else if (key >= '1' && key <= '5') {
      savePet();
      gameMode = false; randomPlayback = false; previewSelect(previewActivities[key-'1']);
    } else if (key == 'n') {
      if (gameMode) {
        for (auto &need : needs) if (!need.pending) { need.pending = true; need.since = gameTime; }
        savePet();
        nextNotice = millis();
      } else { randomPlayback = true; previewRandom(); }
    } else if (gameMode) {
      if (key == 'f') careFor(0);
      if (key == 'w') careFor(1);
      if (key == 's') careFor(2);
      if (key == 'p') careFor(3);
      if (key == ' ') careFor(oldestNeed());
    }
  }
}

void petSetup(bool mounted) {
  filesystemReady = mounted; pinMode(PET_BUTTON_PIN,INPUT_PULLUP);
  loadPet();
  lastTick = millis(); activity = EXERCISING; showPet(); printHelp();
  Serial.println("GAME started: exercising. Care history saved; offline aging paused.");
}

void petLoop() {
  uint32_t now = millis();
  if (gameMode) gameTime += (uint32_t)(now-lastTick);
  lastTick = now; pollButton(now); handleKeyboard(); now = millis();
  bool overlayChanged = false;
  bool needsChanged = false;
  if (gameMode) {
    for (auto &need : needs) if (!need.pending && gameTime >= need.next) {
      need.pending = true; need.since = gameTime; nextNotice = now;
      needsChanged = true;
      Serial.println(need.alert);
      if (PET_BUZZER_PIN >= 0) tone(PET_BUZZER_PIN,2400,120);
    }
    if (activityDuration && now-activityStarted >= activityDuration) {
      if (activity != THUMBS_UP) startActivity(THUMBS_UP,3000);
      else startActivity(EXERCISING,0);
    }
    if (noticeNeed >= 0 && (now-noticeStarted >= 5000 || !needs[noticeNeed].pending)) {
      noticeNeed = -1; overlayChanged = true;
    }
    if (noticeNeed < 0 && reached(now,nextNotice) && oldestNeed() >= 0 && !activityDuration) {
      // Rotate pending requests so hunger cannot hide thirst, sleep, and play.
      for (int i = 0; i < 4; ++i) {
        int index = (noticeCursor+i)%4;
        if (needs[index].pending) { noticeNeed = index; noticeCursor = (index+1)%4; break; }
      }
      noticeStarted = now; nextNotice = now+30000; overlayChanged = true;
    }
  }
  if (needsChanged || (gameTime != lastSavedState.gameTime && now-lastSaveAttempt >= AUTOSAVE_MS)) savePet();
  if (statsScreen) {
    if (now-statsStarted >= 30000) showPet();
    else if (now-lastStats >= 1000) drawStats();
  } else {
    if (!gameMode && randomPlayback && reached(now,previewSwitchAt)) previewRandom();
    playAnimationFrame(overlayChanged);
  }
  delay(1);
}
