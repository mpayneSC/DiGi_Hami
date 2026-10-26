#pragma once
#include <Preferences.h>

constexpr int PET_BUTTON_PIN = 5; // Normally open button between GPIO5 and GND.
constexpr int PET_BUZZER_PIN = -1;
constexpr uint64_t PET_HOUR = 3600000ULL;
enum Activity { IDLE, EATING, DRINKING, SLEEPING, EXERCISING, THUMBS_UP, DANCING };
const char *activityNames[] = {"Idle", "Eating", "Drinking", "Sleeping", "Exercising", "Thumbs up", "Dancing"};
const char *animationPaths[] = {"/idle.gif", "/eating.gif", "/drinking.gif", "/sleeping.gif", "/exercising.gif", "/thumbs-up.gif", "/dance.gif"};
const Activity previewActivities[] = {IDLE, EATING, EXERCISING, SLEEPING, THUMBS_UP, DANCING};
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
bool gameMode = true, autoMode = true;
Activity activity = IDLE;
constexpr uint32_t PET_MINUTE = 60000UL;
constexpr uint32_t AUTO_ALERT_MS = 5000, HAPPY_MS = 6000;
struct StoryStep { Activity scene; uint32_t duration; const char *title; int care; bool health; };
const StoryStep story[] = {
  {IDLE, 30000, "WAKING UP", -1, false},
  {EATING, 7*PET_MINUTE, "BREAKFAST TIME", 0, false},
  {IDLE, 30000, "LET'S EXPLORE", -1, false},
  {EXERCISING, 40*PET_MINUTE, "WHEEL TIME", 3, false},
  {IDLE, 30000, "HEALTH CHECK", -1, true},
  {IDLE, 30000, "EXPLORING", -1, false},
  {DANCING, 2*PET_MINUTE, "DANCE TIME", 3, false},
  {DRINKING, 30000, "WATER BREAK", 1, false},
  {SLEEPING, 30*PET_MINUTE, "NAP TIME", 2, false},
  {IDLE, 30000, "WAKE AND EXPLORE", -1, false},
  {IDLE, 30000, "HEALTH CHECK", -1, true}
};
constexpr int STORY_COUNT = sizeof(story)/sizeof(story[0]);
int storyIndex = -1;
bool storyAlert = false, storyHappy = false;
uint32_t storyStarted = 0;
constexpr uint32_t HAMI_MESSAGE_MS = 6000, HAMI_CHAT_INTERVAL = 15000;
const char *hamiLine1 = nullptr, *hamiLine2 = nullptr;
uint32_t hamiMessageStarted = 0, lastHamiChat = 0;
unsigned hamiChatIndex = 0;
bool hamiMessageVisible = false, hamiMessagePending = false;
uint32_t hamiMessageDue = 0;
constexpr uint32_t FIRST_CHAT_MS = 4000;

void sayHami(const char *first, const char *second, uint32_t wait = 0) {
  hamiLine1 = first; hamiLine2 = second;
  hamiMessageStarted = millis(); lastHamiChat = millis();
  hamiMessageVisible = wait == 0; hamiMessagePending = wait != 0;
  hamiMessageDue = millis()+wait;
  Serial.printf("Hami: %s %s\n",first,second);
}

struct HamiMessage { const char *first, *second; };
const HamiMessage kindWords[] = {
  {"You belong here!", "We're a great team"},
  {"Tiny paws, big love", "Hugs for our class"},
  {"Thanks for caring!", "You help me grow"},
  {"Mistakes are okay", "Let's try again!"},
  {"You can do it!", "One step at a time"},
  {"Kind words help", "Let's use them!"},
  {"I love our class!", "Glad you are here"},
  {"Keep being curious", "What can we learn?"},
  {"Taking turns rocks", "We all get a turn"},
  {"Small steps count", "Look how we grow!"},
  {"You are a kind pal", "Thank you, friend!"},
  {"Let's help a friend", "Kindness is cool!"}
};
// Six short, readable messages per activity; cycle each bank independently.
const HamiMessage activityTalk[][6] = {
  {{"Hello, little world", "What can I find?"},
   {"Sniff, sniff!", "So much to explore"},
   {"Good morning, pals", "Let's learn today"},
   {"What's over there?", "Let's look closely"},
   {"Tiptoe, tiny paws", "An adventure waits"},
   {"I spy something fun", "What do you see?"}},
  {{"Boy, that's good!", "Munch, munch!"},
   {"Yum, yum, yum!", "Thanks for my food"},
   {"Crunch, crunch!", "My tummy is happy"},
   {"Tiny bites for me", "Big happy cheeks!"},
   {"What a tasty treat", "Thank you, friends"},
   {"Mmm, so yummy!", "Fuel for my paws"}},
  {{"Slurp, slurp!", "Ahh, that's good!"},
   {"Sip, sip, hooray!", "Water helps me go"},
   {"Little sips for me", "Happy hamster!"},
   {"Splish, splash!", "Just a tiny sip"},
   {"Fresh water, yay!", "Thanks for my drink"},
   {"Slurpity slurp!", "That hit the spot"}},
  {{"Zzz... zzz...", "Cozy little paws"},
   {"Snore... squeak!", "Dreaming of seeds"},
   {"Zzz... sweet dreams", "Rest helps me grow"},
   {"Curled up and cozy", "See you soon, pals"},
   {"Sleepy little nose", "Zzz... time to rest"},
   {"A soft, happy nap", "Dreaming of play"}},
  {{"Faster, faster!", "Go, little paws!"},
   {"Round and round!", "Wheee! This is fun"},
   {"I can keep trying", "You can try too!"},
   {"One tiny step...", "Then one more!"},
   {"Wiggle and stretch", "Moving feels good"},
   {"Hooray for play!", "We make a fun team"}},
  {{"Two paws up!", "We did it, friends"},
   {"Hip hip hooray!", "Thanks, kind class"},
   {"Happy little Hami", "Big love for you!"},
   {"That felt great!", "Let's smile today"},
   {"You helped me!", "Thank you so much"},
   {"High five, friends", "You're wonderful!"}},
  {{"Wiggle, wiggle!", "Happy little feet"},
   {"Let's do a dance!", "Twirl, little paws"},
   {"Clap, clap, hooray", "What a happy day!"},
   {"Bounce to the beat", "Dance your own way"},
   {"A wiggle and a hop", "We all belong here"},
   {"Dancing with pals", "Makes my heart smile"}}
};
static_assert(sizeof(activityTalk)/sizeof(activityTalk[0]) == DANCING+1, "Every activity needs chatter");
unsigned activityChatIndex[DANCING+1] = {}, kindChatIndex = 0;

void hamiChat() {
  HamiMessage message;
  // Sleep stays quiet and cozy; other scenes mix in classroom encouragement.
  if (activity != SLEEPING && hamiChatIndex % 3 == 2) {
    message = kindWords[kindChatIndex++ % (sizeof(kindWords)/sizeof(kindWords[0]))];
  } else {
    message = activityTalk[activity][activityChatIndex[activity]++ % 6];
  }
  ++hamiChatIndex; sayHami(message.first,message.second);
}
bool statsScreen = false, filesystemReady = false;
uint64_t gameTime = 0;
uint32_t lastTick = 0, frameDue = 0, activityStarted = 0, activityDuration = 0;
uint32_t statsStarted = 0, lastStats = 0;
uint32_t nextNotice = 0, noticeStarted = 0;
int noticeNeed = -1, noticeCursor = 0;
String openAnimation;
bool animationFinished = false;
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
  Serial.println("Pet restored. Offline time paused; starting the waking-up routine.");
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
  animationFinished = false;
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
  centered(autoMode ? "AUTO | tap: back" : (gameMode ? "GAME | tap: back" : "PREVIEW | paused"),214,1,0xBDF7);
  presentFrame(); lastStats = millis();
}

void showStats() { statsScreen = true; statsStarted = millis(); drawStats(); }
void showHappy();
void toggleStats() {
  if (statsScreen) { if (gameMode) showHappy(); else showPet(); }
  else showStats();
}

void startActivity(Activity next, uint32_t duration) {
  activity = next; activityStarted = millis(); activityDuration = duration;
  hamiMessageVisible = false; hamiMessagePending = false;
  lastHamiChat = activityStarted-(HAMI_CHAT_INTERVAL-FIRST_CHAT_MS);
  showPet(); Serial.printf("Activity: %s\n",activityNames[next]);
}

void showHappy() {
  startActivity(THUMBS_UP,HAPPY_MS);
  if (autoMode) { storyHappy = true; storyAlert = false; storyStarted = millis(); }
  noticeNeed = -1; hamiMessageVisible = false;
  // Let the happy GIF play unobstructed. Chatter resumes in the next scene.
  lastHamiChat = millis();
}

void careFor(int index) {
  if (!gameMode || index < 0 || index > 3) return;
  Need &need = needs[index];
  need.pending = false; need.lastCare = gameTime; ++need.count;
  need.next = gameTime+need.interval;
  savePet(); // Commit care immediately, before starting its animation.
  noticeNeed = -1; nextNotice = millis()+15000;
  const uint32_t durations[] = {7*PET_MINUTE, 30000, 30*PET_MINUTE, 40*PET_MINUTE};
  startActivity(need.activity,durations[index]);
  const char *thanks[][2] = {
    {"Thanks for dinner!", "Love you, Hami"},
    {"Thanks for my drink", "You're so kind!"},
    {"Thanks for the rest", "Sweet dreams <3"},
    {"Thanks, play buddy!", "I love our time"}
  };
  sayHami(thanks[index][0],thanks[index][1],FIRST_CHAT_MS);
  Serial.printf("%s cared for; total %u\n",need.label,need.count);
}

void setMode(bool game);

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
    if (gameMode && oldestNeed() >= 0) {
      if (autoMode) setMode(true);
      careFor(oldestNeed());
    }
    else toggleStats();
  }
}

void overlayAlert() {
  if (!gameMode || (noticeNeed < 0 && !storyAlert && !hamiMessageVisible)) { presentFrame(); return; }
  // Save and restore only the banner rows, preserving GIF delta-frame pixels.
  constexpr int TOP = 167, ROWS = 45;
  static uint16_t saved[DISPLAY_WIDTH*ROWS];
  memcpy(saved,frameBuffer+TOP*DISPLAY_WIDTH,sizeof(saved));
  bool chatting = !storyAlert && noticeNeed < 0;
  uint16_t color = chatting ? 0xFBAE : (storyAlert ? 0x87E0 : needs[noticeNeed].color);
  ui.fillRoundRect(39,169,162,40,12,color);
  if (storyAlert) {
    // Activity notices are labels only, separate from Hami's speech bubbles.
    centered(story[storyIndex].title,185,1,0);
  } else {
    hamsterPortrait(58,189,11);
    ui.setTextColor(0); ui.setTextSize(1);
    ui.setCursor(78,178); ui.print(chatting ? hamiLine1 : needs[noticeNeed].alert);
    ui.setCursor(78,194); ui.print(chatting ? hamiLine2 : needs[noticeNeed].hint);
  }
  presentFrame();
  memcpy(frameBuffer+TOP*DISPLAY_WIDTH,saved,sizeof(saved));
}

void playAnimationFrame(bool force = false) {
  if (!gifReady || animationFinished || !reached(millis(),frameDue)) { if (force) overlayAlert(); return; }
  int frameDelay = 0;
  uint32_t started = millis(); frameDrawn = false;
  int result = gif.playFrame(false,&frameDelay);
  if (result < 0) {
    gif.close(); gifReady = false;
    Serial.println("GIF decode failed; try selecting another scene.");
    centered("GIF error",110,2); overlayAlert();
  } else {
    if (frameDrawn || force) overlayAlert();
    if (result == 0) {
      // Keep the final thumbs-up pixels; only other activities loop.
      if (activity == THUMBS_UP) animationFinished = true;
      else gif.reset();
    }
    frameDue = started+(uint32_t)max(frameDelay,10);
  }
}

void previewSelect(Activity next) {
  activityDuration = 0; noticeNeed = -1; activity = next;
  storyAlert = false; hamiMessageVisible = false; hamiMessagePending = false; showPet();
  Serial.printf("Preview: %s\n",activityNames[next]);
}

void announceStory() {
  storyIndex = (storyIndex+1)%STORY_COUNT;
  hamiMessageVisible = false; hamiMessagePending = false;
  storyHappy = false; storyAlert = true; storyStarted = millis(); noticeNeed = -1;
  if (statsScreen) showPet();
  overlayAlert();
  Serial.printf("Autopilot: %s in 5 seconds\n",story[storyIndex].title);
}

bool storyHasHappyEnding() {
  const StoryStep &step = story[storyIndex];
  return step.care >= 0 || step.health;
}

void finishStory() {
  // Both timer completion and N go through the happy ending.
  if (!storyAlert && !storyHappy && storyHasHappyEnding()) showHappy();
  else if (!storyHappy) announceStory();
}

void beginStory() {
  storyAlert = false; storyHappy = false;
  const StoryStep &step = story[storyIndex];
  if (step.care >= 0) {
    careFor(step.care);
    if (activity != step.scene) startActivity(step.scene,0);
  }
  else startActivity(step.scene,0);
  activityDuration = 0;
  storyStarted = millis();
  if (step.health) showStats();
}

bool happyFinished(uint32_t now) {
  // Honor the last frame's display time. Retain a fallback if the GIF is absent.
  return gifReady ? (animationFinished && reached(now,frameDue))
                  : now-activityStarted >= HAPPY_MS;
}

void runStory(uint32_t now) {
  if (storyHappy) {
    if (happyFinished(now)) {
      storyIndex = (storyIndex+1)%STORY_COUNT;
      beginStory(); // No notice or five-second pause after thumbs-up.
    }
    return;
  }
  if (storyAlert) {
    if (now-storyStarted >= AUTO_ALERT_MS) beginStory();
  } else if (now-storyStarted >= story[storyIndex].duration) {
    finishStory();
  }
}

void setMode(bool game) {
  savePet();
  gameMode = true; autoMode = !game; storyAlert = false; storyHappy = false;
  noticeNeed = -1; nextNotice = millis(); hamiMessageVisible = false; lastHamiChat = millis();
  if (game) { startActivity(IDLE,0); sayHami("Let's play together", "Love you, friend!",FIRST_CHAT_MS); }
  else { storyIndex = -1; startActivity(IDLE,0); announceStory(); }
  Serial.println(game ? "GAME: manual care" : "AUTO: story and health timers running");
}

void printHelp() {
  Serial.println("g game | a auto (r alias) | h health | f feed | w water | s sleep | p play");
  Serial.println("1 idle | 2 eating | 3 exercise | 4 sleeping | 5 thumbs-up | 6 dance (manual preview)");
  Serial.println("n: next auto chapter / request all in game | space: answer oldest | ?: help");
  Serial.println("Button GPIO5-GND: tap health/back; hold 800 ms to answer oldest request");
}

void handleKeyboard() {
  while (Serial.available()) {
    char key = Serial.read();
    if (key == 'g' || key == 'G') setMode(true);
    else if (key == 'r' || key == 'a') setMode(false);
    else if (key == 'h') toggleStats();
    else if (key == '?') printHelp();
    else if (key >= '1' && key <= '6') {
      savePet();
      gameMode = false; autoMode = false; previewSelect(previewActivities[key-'1']);
    } else if (key == 'n') {
      if (autoMode) finishStory();
      else if (gameMode) {
        for (auto &need : needs) if (!need.pending) { need.pending = true; need.since = gameTime; }
        savePet();
        nextNotice = millis();
      }
    } else if (gameMode) {
      if (autoMode && (key == 'f' || key == 'w' || key == 's' || key == 'p' || key == ' ')) setMode(true);
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
  lastTick = millis(); setMode(false); printHelp();
  Serial.println("AUTO started: waking up. Care history saved; offline aging paused.");
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
    if (!autoMode && activityDuration) {
      if (activity == THUMBS_UP) { if (happyFinished(now)) startActivity(IDLE,0); }
      else if (now-activityStarted >= activityDuration) showHappy();
    }
    if (!autoMode && noticeNeed >= 0 && (now-noticeStarted >= 5000 || !needs[noticeNeed].pending)) {
      noticeNeed = -1; overlayChanged = true;
    }
    if (!autoMode && noticeNeed < 0 && reached(now,nextNotice) && oldestNeed() >= 0 && !activityDuration) {
      // Rotate pending requests so hunger cannot hide thirst, sleep, and play.
      for (int i = 0; i < 4; ++i) {
        int index = (noticeCursor+i)%4;
        if (needs[index].pending) { noticeNeed = index; noticeCursor = (index+1)%4; break; }
      }
      noticeStarted = now; nextNotice = now+30000; overlayChanged = true;
    }
  }
  if (needsChanged || (gameTime != lastSavedState.gameTime && now-lastSaveAttempt >= AUTOSAVE_MS)) savePet();
  if (autoMode) runStory(now);
  now = millis();
  if (hamiMessagePending && !statsScreen && !storyAlert && reached(now,hamiMessageDue)) {
    hamiMessagePending = false; hamiMessageVisible = true;
    hamiMessageStarted = now; lastHamiChat = now; overlayChanged = true;
  }
  if (hamiMessageVisible && now-hamiMessageStarted >= HAMI_MESSAGE_MS) {
    hamiMessageVisible = false; overlayChanged = true;
  }
  if (gameMode && !hamiMessagePending && !statsScreen && !storyAlert && activity != THUMBS_UP && noticeNeed < 0 &&
      now-lastHamiChat >= HAMI_CHAT_INTERVAL) {
    hamiChat(); overlayChanged = true;
  }
  if (statsScreen) {
    if (now-statsStarted >= 30000) { if (gameMode) showHappy(); else showPet(); }
    else if (now-lastStats >= 1000) drawStats();
  } else {
    playAnimationFrame(overlayChanged);
  }
  delay(1);
}
