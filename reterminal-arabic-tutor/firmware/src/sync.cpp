#include "sync.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <ArduinoJson.h>

namespace {

bool wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("sync: connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

bool alreadyHaveDay(const String& date) {
  String dir = String(SD_CONTENT_DIR) + "/" + date;
  return SD.exists(dir.c_str());
}

bool downloadFile(const String& url, const String& destPath) {
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != 200) {
    Serial.printf("sync: GET %s failed (%d)\n", url.c_str(), code);
    http.end();
    return false;
  }
  File f = SD.open(destPath.c_str(), FILE_WRITE);
  if (!f) {
    Serial.printf("sync: cannot open %s for write\n", destPath.c_str());
    http.end();
    return false;
  }
  http.writeToStream(&f);
  f.close();
  http.end();
  return true;
}

} // namespace

namespace sync {

bool checkForNewContent() {
  if (!wifiConnect()) {
    Serial.println("sync: WiFi connect failed, staying on cached content");
    return false;
  }

  HTTPClient http;
  http.begin(CONTENT_MANIFEST_URL);
  int code = http.GET();
  if (code != 200) {
    Serial.printf("sync: manifest GET failed (%d)\n", code);
    http.end();
    WiFi.disconnect(true);
    return false;
  }

  String body = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("sync: manifest parse error: %s\n", err.c_str());
    WiFi.disconnect(true);
    return false;
  }

  String date = doc["date"] | "";
  if (date.isEmpty()) {
    Serial.println("sync: manifest missing date");
    WiFi.disconnect(true);
    return false;
  }

  if (alreadyHaveDay(date)) {
    Serial.println("sync: already have today's content");
    WiFi.disconnect(true);
    return false;
  }

  // Figure out the base URL to resolve relative filenames against.
  String manifestUrl = CONTENT_MANIFEST_URL;
  String baseUrl = manifestUrl.substring(0, manifestUrl.lastIndexOf('/') + 1);

  String dir = String(SD_CONTENT_DIR) + "/" + date;
  SD.mkdir(dir.c_str());

  // Save the manifest itself.
  {
    File mf = SD.open((dir + "/manifest.json").c_str(), FILE_WRITE);
    if (mf) {
      mf.print(body);
      mf.close();
    }
  }

  bool ok = true;
  for (JsonVariant v : doc["reading_pages"].as<JsonArray>()) {
    String name = v.as<const char*>();
    ok &= downloadFile(baseUrl + name, dir + "/" + name);
  }
  for (JsonObject c : doc["vocab_cards"].as<JsonArray>()) {
    String front = c["front"].as<String>();
    String back = c["back"].as<String>();
    ok &= downloadFile(baseUrl + front, dir + "/" + front);
    ok &= downloadFile(baseUrl + back, dir + "/" + back);
  }

  WiFi.disconnect(true);

  if (!ok) {
    Serial.println("sync: one or more files failed — content may be incomplete");
  }
  return ok;
}

} // namespace sync
