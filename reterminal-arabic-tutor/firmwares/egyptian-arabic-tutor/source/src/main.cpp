#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "hal_display.h"
#include "hal_touch.h"
#include "hal_buzzer.h"
#include "content.h"
#include "sync.h"
#include "trainer.h"

enum class Mode { HOME, READING, TRAINER };

Mode mode = Mode::HOME;
int readingPageIdx = 0;
DayContent today;

unsigned long lastSyncCheck = 0;

void drawReadingPage() {
  content::showReadingPage(today, readingPageIdx);
}

void drawCurrentMode() {
  switch (mode) {
    case Mode::HOME:    content::showHomePage(today); break;
    case Mode::READING: drawReadingPage(); break;
    case Mode::TRAINER:
      if (!today.vocabCards.empty()) {
        content::showVocabCard(today, trainer::currentIndex(), false);
      }
      break;
  }
}

// The on-screen menu bar is baked into every bitmap (see content.h), so the
// firmware doesn't draw it — it just needs to know a tap in the bottom
// MENU_BAR_HEIGHT pixels is a menu tap, split into three equal thirds.
// This check runs BEFORE any mode-specific touch handling, every frame,
// so the menu works no matter what's currently on screen.
bool handleMenuBarTap(const touch::Point& p) {
  if (!p.pressed || p.y < (SCREEN_HEIGHT - MENU_BAR_HEIGHT)) return false;

  Mode tapped;
  if (p.x < SCREEN_WIDTH / 3) tapped = Mode::HOME;
  else if (p.x < 2 * SCREEN_WIDTH / 3) tapped = Mode::READING;
  else tapped = Mode::TRAINER;

  if (tapped != mode) {
    mode = tapped;
    drawCurrentMode();
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nreTerminal Sticky — Arabic Tutor");

  display::init();
  touch::init();
  buzzer::init();

  if (!SD.begin()) {
    Serial.println("setup: SD card init failed — check card is inserted");
  }

  trainer::init();

  Serial.println("setup: checking for new content...");
  bool gotNew = sync::checkForNewContent();
  if (gotNew) buzzer::beepSync();

  today = content::loadLatest();
  if (today.valid) {
    Serial.printf("setup: loaded content for %s (%s)\n",
                   today.date.c_str(), today.title.c_str());
    drawCurrentMode(); // opens on Home
  } else {
    Serial.println("setup: no content on SD card yet — push a day package");
  }

  lastSyncCheck = millis();
}

void loop() {
  // Periodic re-sync without needing a reboot.
  if ((millis() - lastSyncCheck) > (SYNC_INTERVAL_SECONDS * 1000UL)) {
    lastSyncCheck = millis();
    if (sync::checkForNewContent()) {
      buzzer::beepSync();
      today = content::loadLatest();
      readingPageIdx = 0;
      drawCurrentMode();
    }
  }

  touch::Point p = touch::poll();
  char swipe = touch::pollSwipe();

  // Menu bar always wins, regardless of mode — this is what lets you jump
  // straight Home or into the trainer without backing out of reading first.
  if (handleMenuBarTap(p)) {
    delay(400); // crude debounce — e-ink refresh is slow enough this rarely matters
    delay(20);
    return;
  }

  // Everything below here only fires for taps in the content area above
  // the menu bar (handleMenuBarTap already returned for menu taps).
  if (mode == Mode::READING && p.pressed) {
    // Left/right half of the content area pages back/forward.
    if (p.x < SCREEN_WIDTH / 2) {
      readingPageIdx = max(0, readingPageIdx - 1);
    } else {
      readingPageIdx = min((int)today.readingPages.size() - 1, readingPageIdx + 1);
    }
    drawReadingPage();
    delay(400);
  } else if (mode == Mode::TRAINER) {
    trainer::update(today, p, swipe);
  }
  // Mode::HOME has no content-area interaction — menu bar is the only way
  // to navigate from it, which matches showing it as a simple landing page.

  delay(20);
}
