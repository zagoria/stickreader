#pragma once

// ---- WiFi ----
#define WIFI_SSID     "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// ---- Content source ----
// Set this to your GitHub Pages URL once automation/ is set up, e.g.
// "https://yourusername.github.io/reterminal-arabic-tutor/content/latest/manifest.json"
#define CONTENT_MANIFEST_URL "https://yourusername.github.io/REPO/content/latest/manifest.json"

// How often to check for new content (device wakes, checks, goes back to
// sleep). Default: once a day.
#define SYNC_INTERVAL_SECONDS (24UL * 60UL * 60UL)

// ---- Display ----
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480

// Bottom menu bar (Home / Read / Train) is baked into every bitmap by
// content-pipeline/generate_day.py — this height MUST match MENU_H there,
// since the firmware uses it to know which taps are menu taps vs. content
// taps, but doesn't draw the bar itself (it's just pixels in the bitmap).
#define MENU_BAR_HEIGHT 56

// ---- SD card paths ----
#define SD_CONTENT_DIR   "/content"
#define SD_PROGRESS_FILE "/progress.json"

// ---- Pin mapping ----
// Taken from published open-source firmware for this exact board
// (Lukilyy/reterminal-sticky-2048-eink-game, limengdu/reTerminal_Sticky_Bunny).
// Cross-check against whichever driver source you pull in per
// README_DRIVERS.md — pin numbers should match, but verify before flashing.
#define PIN_BTN_POWER        4   // AI / power button
#define PIN_PWR_DETECT        9   // external power detect
#define PIN_EPD_SPI_START     13  // E-paper SPI pins occupy 13-18
#define PIN_GT911_SDA         2
#define PIN_GT911_SCL         3
#define PIN_GT911_EN          42
#define PIN_GT911_INT         21
#define PIN_GT911_RST         41
#define PIN_BATT_SDA          0   // BQ27220 fuel gauge I2C
#define PIN_BATT_SCL          1
#define PIN_CHARGER_EN        39
#define PIN_BUZZER             48
#define PIN_PWR_HOLD           45
#define PIN_PWR_LOCK            46
