#include "SerialShell.h"

void SerialShell::begin() {
  index = 0;
}

void SerialShell::update(AppState &state, AppStats &stats, GpsManager &gps, StorageManager &storage,
                         WifiScanner &wifi, DisplayUi &display, BleScanner &ble) {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (index > 0) {
        buffer[index] = '\0';
        handleLine(String(buffer), state, stats, gps, storage, wifi, display, ble);
        index = 0;
      }
    } else if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
  }
}

void SerialShell::printHelp() {
  Serial.println(F("Commands: help, status, gps, scan, log on|off, fast on|off, ble on|off, rotate"));
}

void SerialShell::handleLine(String line, AppState &state, AppStats &stats, GpsManager &gps,
                             StorageManager &storage, WifiScanner &wifi, DisplayUi &display, BleScanner &ble) {
  line.trim();
  line.toLowerCase();

  if (line == "help") {
    printHelp();
  } else if (line == "status") {
    printStatus(state, stats, gps, storage);
  } else if (line == "gps") {
    printGps(gps);
  } else if (line == "scan") {
    state.immediateScan = true;
    wifi.forceScan();
    Serial.println(F("Manual scan queued"));
  } else if (line == "log on") {
    state.loggingEnabled = true;
    display.flashMessage("Logging enabled");
    Serial.println(F("Logging enabled"));
  } else if (line == "log off") {
    state.loggingEnabled = false;
    display.flashMessage("Logging paused");
    Serial.println(F("Logging paused"));
  } else if (line == "fast on") {
    state.fastScan = true;
    Serial.println(F("Fast scan enabled"));
  } else if (line == "fast off") {
    state.fastScan = false;
    Serial.println(F("Fast scan disabled"));
  } else if (line == "ble on") {
    if (ble.available()) {
      state.bleEnabled = true;
      Serial.println(F("BLE scanner enabled"));
    } else {
      Serial.println(F("BLE scanner not compiled in"));
    }
  } else if (line == "ble off") {
    state.bleEnabled = false;
    Serial.println(F("BLE scanner disabled"));
  } else if (line == "rotate") {
    if (storage.rotate(gps.fix())) {
      stats.rotations++;
      Serial.print(F("Rotated log to "));
      Serial.println(storage.wifiLogPath());
    } else {
      stats.sdWriteFailures++;
      Serial.println(F("Log rotation failed"));
    }
  } else {
    Serial.print(F("Unknown command: "));
    Serial.println(line);
    printHelp();
  }
}

void SerialShell::printStatus(const AppState &state, const AppStats &stats, GpsManager &gps, StorageManager &storage) {
  FixData fix = gps.fix();
  Serial.println(F("ESP32 Wardriver Pro status"));
  Serial.print(F("Logging: "));
  Serial.println(state.loggingEnabled ? F("enabled") : F("paused"));
  Serial.print(F("Fast scan: "));
  Serial.println(state.fastScan ? F("enabled") : F("disabled"));
  Serial.print(F("BLE: "));
  Serial.println(state.bleEnabled ? F("enabled") : F("disabled"));
  Serial.print(F("GPS fresh: "));
  Serial.println(fix.fresh ? F("yes") : F("no"));
  Serial.print(F("Satellites: "));
  Serial.println(fix.satellites);
  Serial.print(F("Scans: "));
  Serial.println(stats.scans);
  Serial.print(F("Rows: "));
  Serial.println(stats.rowsWritten);
  Serial.print(F("Unique BSSIDs: "));
  Serial.println(stats.uniqueNetworks);
  Serial.print(F("WiFi log: "));
  Serial.println(storage.wifiLogPath().length() ? storage.wifiLogPath() : String("none"));
  Serial.print(F("Summary: "));
  Serial.println(storage.summaryPath().length() ? storage.summaryPath() : String("none"));
  Serial.print(F("Last error: "));
  Serial.println(stats.lastError.length() ? stats.lastError : String("none"));
}

void SerialShell::printGps(GpsManager &gps) {
  FixData fix = gps.fix();
  Serial.print(F("Fresh: "));
  Serial.println(fix.fresh ? F("yes") : F("no"));
  Serial.print(F("Satellites: "));
  Serial.println(fix.satellites);
  Serial.print(F("Latitude: "));
  Serial.println(fix.latitude, 6);
  Serial.print(F("Longitude: "));
  Serial.println(fix.longitude, 6);
  Serial.print(F("Altitude m: "));
  Serial.println(fix.altitudeMeters, 1);
  Serial.print(F("HDOP: "));
  Serial.println(fix.hdop, 1);
  Serial.print(F("Timestamp: "));
  Serial.println(fix.timestamp);
  Serial.print(F("GPS chars: "));
  Serial.println(fix.charsProcessed);
}
