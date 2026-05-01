#include "DisplayUi.h"

DisplayUi::DisplayUi() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN) {
}

bool DisplayUi::begin() {
  bool ok = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!ok) {
    Serial.println(F("SSD1306 allocation failed"));
    return false;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.display();
  return true;
}

void DisplayUi::update(const FixData &fix, const AppStats &stats, const AppState &state, const StorageManager &storage) {
  if (millis() - lastRenderMs < 250) {
    return;
  }
  lastRenderMs = millis();

  if (millis() - lastPageMs > 7000) {
    nextPage();
  }

  display.clearDisplay();

  if (flashText.length() && millis() < flashUntilMs) {
    header("WARD DRIVER", state);
    display.setCursor(0, 24);
    display.setTextSize(1);
    display.println(flashText);
    display.display();
    return;
  }

  switch (page) {
    case 0:
      renderGps(fix, stats, state);
      break;
    case 1:
      renderStats(stats, state);
      break;
    case 2:
      renderTopAps(stats, state);
      break;
    default:
      renderStorage(storage, stats, state);
      break;
  }

  display.display();
}

void DisplayUi::nextPage() {
  page = (page + 1) % 4;
  lastPageMs = millis();
}

void DisplayUi::flashMessage(const String &message) {
  flashText = message;
  flashUntilMs = millis() + 1500;
}

void DisplayUi::header(const char *title, const AppState &state) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(title);
  display.setCursor(92, 0);
  if (!state.loggingEnabled) {
    display.print("PAUSE");
  } else if (state.fastScan) {
    display.print("FAST");
  } else {
    display.print("LOG");
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void DisplayUi::renderGps(const FixData &fix, const AppStats &stats, const AppState &state) {
  header("GPS", state);
  display.setCursor(0, 14);
  display.print("Fix: ");
  display.print(fix.fresh ? "fresh" : "waiting");
  display.print(" Sat:");
  display.println(fix.satellites);
  display.print("Lat ");
  display.println(fix.latitude, 5);
  display.print("Lng ");
  display.println(fix.longitude, 5);
  display.print("HDOP ");
  display.print(fix.hdop, 1);
  display.print(" Age ");
  display.print(fix.ageMs == UINT32_MAX ? 0 : fix.ageMs / 1000);
  display.println("s");
  display.print("Scans ");
  display.print(stats.scans);
  display.print(" Rows ");
  display.println(stats.rowsWritten);
}

void DisplayUi::renderStats(const AppStats &stats, const AppState &state) {
  header("SESSION", state);
  display.setCursor(0, 14);
  display.print("Seen ");
  display.print(stats.networksSeen);
  display.print(" Unique ");
  display.println(stats.uniqueNetworks);
  display.print("Open ");
  display.print(stats.openNetworks);
  display.print(" Sec ");
  display.println(stats.securedNetworks);
  display.print("Hidden ");
  display.print(stats.hiddenNetworks);
  display.print(" BLE ");
  display.println(stats.bleBeacons);
  display.print("GPS waits ");
  display.println(stats.gpsWaits);
  display.print("SD errors ");
  display.println(stats.sdWriteFailures);
}

void DisplayUi::renderTopAps(const AppStats &stats, const AppState &state) {
  header("TOP APs", state);
  display.setCursor(0, 14);
  for (uint8_t i = 0; i < TOP_AP_COUNT; i++) {
    if (stats.topAps[i].bssid.length() == 0) {
      display.println("--");
      continue;
    }
    display.print(stats.topAps[i].rssi);
    display.print(" ch");
    display.print(stats.topAps[i].channel);
    display.print(" ");
    printClipped(stats.topAps[i].ssid.length() ? stats.topAps[i].ssid : String("<hidden>"), 10);
    display.println();
  }
}

void DisplayUi::renderStorage(const StorageManager &storage, const AppStats &stats, const AppState &state) {
  header("STORAGE", state);
  display.setCursor(0, 14);
  display.print("SD: ");
  display.println(storage.ready() ? "ready" : "missing");
  display.print("File: ");
  printClipped(storage.wifiLogPath().length() ? storage.wifiLogPath() : String("none"), 15);
  display.println();
  display.print("Bytes: ");
  display.println(storage.wifiLogBytes());
  display.print("Rows: ");
  display.println(stats.rowsWritten);
  display.print("Err: ");
  printClipped(stats.lastError.length() ? stats.lastError : String("none"), 16);
}

void DisplayUi::printClipped(const String &value, uint8_t maxChars) {
  for (uint8_t i = 0; i < value.length() && i < maxChars; i++) {
    display.print(value.charAt(i));
  }
}
