#include "content.h"
#include "config.h"
#include "hal_display.h"
#include <SD.h>
#include <ArduinoJson.h>

namespace {

bool readBitmap(const String& path, int& w, int& h, std::vector<uint8_t>& out) {
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) {
    Serial.printf("content: missing bitmap %s\n", path.c_str());
    return false;
  }
  // Minimal custom format written by generate_day.py (not a general .bmp
  // parser — the pipeline writes width/height as two little-endian uint16s
  // followed by raw packed 1bpp rows, to keep the device-side parser tiny).
  uint8_t hdr[4];
  if (f.read(hdr, 4) != 4) { f.close(); return false; }
  w = hdr[0] | (hdr[1] << 8);
  h = hdr[2] | (hdr[3] << 8);
  size_t rowBytes = (w + 7) / 8;
  size_t total = rowBytes * h;
  out.resize(total);
  f.read(out.data(), total);
  f.close();
  return true;
}

void drawBitmapFile(const String& path, int x, int y) {
  int w, h;
  std::vector<uint8_t> data;
  if (readBitmap(path, w, h, data)) {
    display::drawBitmap1bpp(x, y, w, h, data.data());
  }
}

} // namespace

namespace content {

DayContent loadLatest() {
  DayContent day;
  File root = SD.open(SD_CONTENT_DIR);
  if (!root) {
    Serial.println("content: /content not found on SD card");
    return day;
  }

  String newest;
  File entry = root.openNextFile();
  while (entry) {
    if (entry.isDirectory()) {
      String name = entry.name();
      if (name > newest) newest = name;
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();

  if (newest.isEmpty()) {
    Serial.println("content: no day folders found");
    return day;
  }

  day.dir = String(SD_CONTENT_DIR) + "/" + newest;
  String manifestPath = day.dir + "/manifest.json";

  File mf = SD.open(manifestPath.c_str(), FILE_READ);
  if (!mf) {
    Serial.printf("content: missing manifest at %s\n", manifestPath.c_str());
    return day;
  }

  JsonDocument doc; // ArduinoJson v7 auto-sizing document
  DeserializationError err = deserializeJson(doc, mf);
  mf.close();
  if (err) {
    Serial.printf("content: manifest parse error: %s\n", err.c_str());
    return day;
  }

  day.date = doc["date"] | "";
  day.title = doc["title"] | "";
  day.homePage = doc["home_page"] | "";

  for (JsonVariant v : doc["reading_pages"].as<JsonArray>()) {
    day.readingPages.push_back(String(v.as<const char*>()));
  }
  for (JsonObject c : doc["vocab_cards"].as<JsonArray>()) {
    VocabCard card;
    card.front = c["front"].as<String>();
    card.back = c["back"].as<String>();
    day.vocabCards.push_back(card);
  }

  day.valid = true;
  return day;
}

void showHomePage(const DayContent& day) {
  if (!day.valid || day.homePage.isEmpty()) return;
  display::clearBuffer();
  drawBitmapFile(day.dir + "/" + day.homePage, 0, 0);
  display::refresh(/*fullRefresh=*/true);
}

void showReadingPage(const DayContent& day, int index) {
  if (!day.valid || index < 0 || index >= (int)day.readingPages.size()) return;
  display::clearBuffer();
  String path = day.dir + "/" + day.readingPages[index];
  drawBitmapFile(path, 0, 0);
  display::refresh(/*fullRefresh=*/true);
}

void showVocabCard(const DayContent& day, int index, bool showBack) {
  if (!day.valid || index < 0 || index >= (int)day.vocabCards.size()) return;
  display::clearBuffer();
  const VocabCard& card = day.vocabCards[index];
  String path = day.dir + "/" + (showBack ? card.back : card.front);
  drawBitmapFile(path, 0, 0);
  // Partial refresh is much faster for flipping through flashcards, if your
  // chosen driver supports it (see README_DRIVERS.md).
  display::refresh(/*fullRefresh=*/false);
}

} // namespace content
