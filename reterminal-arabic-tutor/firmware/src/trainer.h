#pragma once
#include "content.h"

namespace trainer {
  // Loads/creates /progress.json on the SD card.
  void init();

  // Call once per loop iteration while in trainer mode. Handles touch input
  // (tap = flip, swipe left/right = next/prev) and draws the current card.
  // Call content::showVocabCard internally.
  void update(const DayContent& day);

  // Mark the currently-shown card known / still-learning, advance to next.
  void markKnown(bool known);

  int currentIndex();
  int knownCount();
  int learningCount();
}
