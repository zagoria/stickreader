#pragma once
#include <Arduino.h>

// Thin wrapper over the PWM buzzer on PIN_BUZZER. Simple enough that you
// likely don't need to borrow this one from an upstream repo — a basic
// ledcWriteTone()-based implementation is normally enough. Kept as a HAL
// anyway so main.cpp never touches hardware directly.

namespace buzzer {
  void init();
  void beepKnown();    // short high beep — "knew it" in the trainer
  void beepLearning(); // short low beep — "still learning"
  void beepSync();     // two quick beeps — new content downloaded
}
