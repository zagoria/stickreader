#pragma once
#include <Arduino.h>

// Hardware abstraction for the SSD1677 e-ink panel.
//
// IMPLEMENT THESE in hal_display.cpp using a verified driver for this exact
// panel — see ../README_DRIVERS.md. The app logic in main.cpp / content.cpp
// only calls through this interface, so swapping in a real driver means
// filling in hal_display.cpp and never touching the rest of the firmware.

namespace display {

  // Initialize SPI, reset the panel, run the SSD1677 init sequence.
  void init();

  // Clear the whole screen to white. Call before drawing a new page.
  void clearBuffer();

  // Draw a 1-bit bitmap (MSB-first packed rows, like a standard .bmp with
  // 1 bit per pixel) into the internal framebuffer at (x, y).
  // w and h are in pixels; data length must be ceil(w/8) * h bytes.
  void drawBitmap1bpp(int x, int y, int w, int h, const uint8_t* data);

  // Push the framebuffer to the panel. Use fullRefresh sparingly (slow,
  // flashes) — use partial refresh for flashcard flips where supported.
  void refresh(bool fullRefresh);

  // Put panel into its lowest-power hold state between updates.
  void sleep();

} // namespace display
