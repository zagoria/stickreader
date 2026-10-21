#pragma once
#include "content.h"
#include "hal_touch.h"

namespace trainer {
  // Loads/creates /progress.json on the SD card.
  void init();

  // Call once per loop iteration while in trainer mode, but only when the
  // tap/swipe did NOT land in the bottom menu bar — main.cpp checks that
  // first so the menu always works regardless of mode. Handles touch input
  // (tap = flip, swipe left/right = next/prev) and draws the current card.
  void update(const DayContent& day, const touch::Point& p, char swipe);

  // Mark the currently-shown card known / still-learning, advance to next.
  void markKnown(bool known);

  int currentIndex();
  int knownCount();
  int learningCount();
}
