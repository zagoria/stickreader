#include "trainer.h"
#include "config.h"
#include "hal_touch.h"
#include "hal_buzzer.h"
#include <SD.h>
#include <ArduinoJson.h>

namespace {
  int idx = 0;
  bool showingBack = false;
  int known = 0;
  int learning = 0;

  void saveProgress() {
    JsonDocument doc;
    doc["known"] = known;
    doc["learning"] = learning;
    File f = SD.open(SD_PROGRESS_FILE, FILE_WRITE);
    if (f) {
      serializeJson(doc, f);
      f.close();
    }
  }

  void loadProgress() {
    File f = SD.open(SD_PROGRESS_FILE, FILE_READ);
    if (!f) return;
    JsonDocument doc;
    if (!deserializeJson(doc, f)) {
      known = doc["known"] | 0;
      learning = doc["learning"] | 0;
    }
    f.close();
  }
}

namespace trainer {

void init() {
  loadProgress();
}

void update(const DayContent& day) {
  if (day.vocabCards.empty()) return;

  char swipe = touch::pollSwipe();
  touch::Point p = touch::poll();

  bool needsRedraw = false;

  if (swipe == 'R') {
    idx = (idx + 1) % day.vocabCards.size();
    showingBack = false;
    needsRedraw = true;
  } else if (swipe == 'L') {
    idx = (idx - 1 + day.vocabCards.size()) % day.vocabCards.size();
    showingBack = false;
    needsRedraw = true;
  } else if (p.pressed) {
    // A plain tap (not part of a swipe) flips the card.
    showingBack = !showingBack;
    needsRedraw = true;
  }

  if (needsRedraw) {
    content::showVocabCard(day, idx, showingBack);
  }
}

void markKnown(bool isKnown) {
  if (isKnown) { known++; buzzer::beepKnown(); }
  else { learning++; buzzer::beepLearning(); }
  saveProgress();
}

int currentIndex() { return idx; }
int knownCount() { return known; }
int learningCount() { return learning; }

} // namespace trainer
