#pragma once
#include <Arduino.h>

// Hardware abstraction for the GT911 capacitive touch controller.
// IMPLEMENT in hal_touch.cpp — see ../README_DRIVERS.md.

namespace touch {

  struct Point {
    int x;
    int y;
    bool pressed;
  };

  void init();

  // Non-blocking: returns the current touch point (pressed=false if none).
  Point poll();

  // Simple swipe detection used by the trainer view. Returns 'L', 'R', or
  // 0 if no swipe was completed since the last call.
  char pollSwipe();

} // namespace touch
