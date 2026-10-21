#pragma once
#include <Arduino.h>

namespace sync {
  // Connects to WiFi, fetches CONTENT_MANIFEST_URL, and if its "date" field
  // is newer than anything already cached under /content, downloads the
  // listed bitmap files into /content/<date>/ on the SD card.
  // Returns true if new content was downloaded.
  bool checkForNewContent();
}
