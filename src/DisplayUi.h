#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#include "Config.h"
#include "StorageManager.h"
#include "WardriverTypes.h"

class DisplayUi {
public:
  DisplayUi();
  bool begin();
  void update(const FixData &fix, const AppStats &stats, const AppState &state, const StorageManager &storage);
  void nextPage();
  void flashMessage(const String &message);

private:
  Adafruit_SSD1306 display;
  uint8_t page = 0;
  uint32_t lastRenderMs = 0;
  uint32_t lastPageMs = 0;
  uint32_t flashUntilMs = 0;
  String flashText;

  void header(const char *title, const AppState &state);
  void renderGps(const FixData &fix, const AppStats &stats, const AppState &state);
  void renderStats(const AppStats &stats, const AppState &state);
  void renderTopAps(const AppStats &stats, const AppState &state);
  void renderStorage(const StorageManager &storage, const AppStats &stats, const AppState &state);
  void printClipped(const String &value, uint8_t maxChars);
};

#endif
