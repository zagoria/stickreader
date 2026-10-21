#pragma once
#include <Arduino.h>

// A "day package" on the SD card looks like:
//
//   /content/2026-10-14/manifest.json
//   /content/2026-10-14/reading_01.bmp
//   /content/2026-10-14/reading_02.bmp
//   ...
//   /content/2026-10-14/vocab_01_front.bmp
//   /content/2026-10-14/vocab_01_back.bmp
//   ...
//
// manifest.json:
// {
//   "date": "2026-10-14",
//   "title": "Ahwa culture",
//   "reading_pages": ["reading_01.bmp", "reading_02.bmp"],
//   "vocab_cards": [
//     {"front": "vocab_01_front.bmp", "back": "vocab_01_back.bmp"}
//   ]
// }
//
// Bitmaps are pre-rendered 1bpp images sized to fit the 800x480 panel,
// produced by content-pipeline/generate_day.py — this sidesteps doing
// Arabic contextual letter-shaping on-device, which is impractical on an
// ESP32.

struct VocabCard {
  String front;
  String back;
};

struct DayContent {
  String date;
  String title;
  String dir; // e.g. "/content/2026-10-14"
  std::vector<String> readingPages;
  std::vector<VocabCard> vocabCards;
  bool valid = false;
};

namespace content {

  // Reads /content/<latest-dir-by-name>/manifest.json — day folders sort
  // lexically (ISO dates do), so the newest is just the last one.
  DayContent loadLatest();

  // Draws page `index` of the reading text to the framebuffer and refreshes.
  void showReadingPage(const DayContent& day, int index);

  // Draws the front or back of vocab card `index`.
  void showVocabCard(const DayContent& day, int index, bool showBack);

} // namespace content
