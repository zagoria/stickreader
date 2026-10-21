#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "hal_display.h"
#include "hal_touch.h"
#include "hal_buzzer.h"
#include "content.h"
#include "sync.h"
#include "trainer.h"

enum class Mode { READING, TRAINER };

Mode mode = Mode::READING;
int readingPageIdx = 0;
DayContent today;

unsigned long lastSyncCheck = 0;

void drawReadingPage() {
  content::showReadingPage(today, readingPageIdx);
}

void handlePowerButtonTap() {
  // Short tap on the power/AI button toggles between reading and trainer.
  if (mode == Mode::READING) {
    mode = Mode::TRAINER;
    if (!today.vocabCards.empty()) {
      content::showVocabCard(today, trainer::currentIndex(), false);
    }
  } else {
    mode = Mode::READING;
    drawReadingPage();
  }
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
    drawReadingPage();
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
      if (mode == Mode::READING) drawReadingPage();
    }
  }

  touch::Point p = touch::poll();

  // Reading mode: tap left/right half of screen to page back/forward.
  // (Swap in a real swipe gesture here if your touch driver supports one —
  // see hal_touch.h.)
  if (mode == Mode::READING && p.pressed) {
    if (p.x < SCREEN_WIDTH / 2) {
      readingPageIdx = max(0, readingPageIdx - 1);
    } else {
      readingPageIdx = min((int)today.readingPages.size() - 1, readingPageIdx + 1);
    }
    drawReadingPage();
    delay(400); // crude debounce — e-ink refresh is slow enough this rarely matters
  }

  if (mode == Mode::TRAINER) {
    trainer::update(today);
  }

  // TODO: wire PIN_BTN_POWER to call handlePowerButtonTap() on a short
  // press (long press can stay as the hardware's own power toggle).
  // Left as a TODO because the exact press/hold timing convention for this
  // button is worth matching to Seeed's own firmware rather than guessing.

  delay(20);
}
