#include "StorageManager.h"

bool StorageManager::begin() {
  sdReady = SD.begin(SD_CS_PIN);
  return sdReady;
}

bool StorageManager::ready() const {
  return sdReady;
}

const String &StorageManager::wifiLogPath() const {
  return activeWifiLog;
}

const String &StorageManager::summaryPath() const {
  return activeSummary;
}

uint32_t StorageManager::wifiLogBytes() const {
  if (!sdReady || activeWifiLog.length() == 0 || !SD.exists(activeWifiLog)) {
    return 0;
  }

  File file = SD.open(activeWifiLog, FILE_READ);
  if (!file) {
    return 0;
  }
  uint32_t size = file.size();
  file.close();
  return size;
}

bool StorageManager::ensureWifiLog(const FixData &fix) {
  if (!sdReady) {
    return false;
  }

  if (activeWifiLog.length() == 0) {
    return createWifiLog(fix);
  }

  if (wifiLogBytes() >= LOG_ROTATE_BYTES) {
    return rotate(fix);
  }

  return true;
}

bool StorageManager::rotate(const FixData &fix) {
  activeWifiLog = "";
  activeSummary = "";
  rowsSinceFlush = 0;
  return createWifiLog(fix);
}

bool StorageManager::createWifiLog(const FixData &fix) {
  char path[40];
  snprintf(path, sizeof(path), "/wigle_%s.csv", fix.filenameStamp);
  activeWifiLog = String(path);

  char summary[40];
  snprintf(summary, sizeof(summary), "/summary_%s.txt", fix.filenameStamp);
  activeSummary = String(summary);

  File file = SD.open(activeWifiLog, FILE_WRITE);
  if (!file) {
    activeWifiLog = "";
    activeSummary = "";
    return false;
  }

  bool ok = appendHeader(file);
  file.close();
  return ok;
}

bool StorageManager::appendHeader(File &file) {
  file.println(F("WigleWifi-1.4,appRelease=ESP32WardriverPro,model=ESP32,release=1.0,device=ESP32,display=SSD1306,board=esp32"));
  file.println(F("MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type"));
  return file.getWriteError() == 0;
}

bool StorageManager::writeWifiRecord(const WifiNetworkRecord &record, const FixData &fix) {
  if (!ensureWifiLog(fix)) {
    return false;
  }

  File file = SD.open(activeWifiLog, FILE_APPEND);
  if (!file) {
    return false;
  }

  file.print(csvEscape(record.bssid));
  file.print(',');
  file.print(csvEscape(record.ssid.length() == 0 ? String("<hidden>") : record.ssid));
  file.print(',');
  file.print(csvEscape(record.authMode));
  file.print(',');
  file.print(csvEscape(String(fix.timestamp)));
  file.print(',');
  file.print(record.channel);
  file.print(',');
  file.print(record.frequencyMHz);
  file.print(',');
  file.print(record.rssi);
  file.print(',');
  file.print(fix.latitude, 6);
  file.print(',');
  file.print(fix.longitude, 6);
  file.print(',');
  file.print(fix.altitudeMeters, 1);
  file.print(',');
  file.print(fix.hdop, 1);
  file.println(F(",WIFI"));

  rowsSinceFlush++;
  if (rowsSinceFlush >= LOG_FLUSH_INTERVAL_ROWS) {
    file.flush();
    rowsSinceFlush = 0;
  }

  bool ok = file.getWriteError() == 0;
  file.close();
  return ok;
}

void StorageManager::maybeWriteSummary(const FixData &fix, const AppStats &stats, const AppState &state) {
  if (!sdReady || activeSummary.length() == 0 || millis() - lastSummaryMs < SUMMARY_WRITE_INTERVAL_MS) {
    return;
  }

  appendSummary(fix, stats, state);
  lastSummaryMs = millis();
}

bool StorageManager::appendSummary(const FixData &fix, const AppStats &stats, const AppState &state) {
  File file = SD.open(activeSummary, FILE_WRITE);
  if (!file) {
    return false;
  }

  file.println(F("ESP32 Wardriver Pro Session"));
  file.print(F("Updated: "));
  file.println(fix.timestamp);
  file.print(F("WiFi log: "));
  file.println(activeWifiLog);
  file.print(F("Logging: "));
  file.println(state.loggingEnabled ? F("enabled") : F("paused"));
  file.print(F("Fast scan: "));
  file.println(state.fastScan ? F("enabled") : F("disabled"));
  file.print(F("Scans: "));
  file.println(stats.scans);
  file.print(F("Rows written: "));
  file.println(stats.rowsWritten);
  file.print(F("Networks seen: "));
  file.println(stats.networksSeen);
  file.print(F("Unique BSSIDs: "));
  file.println(stats.uniqueNetworks);
  file.print(F("Open networks: "));
  file.println(stats.openNetworks);
  file.print(F("Secured networks: "));
  file.println(stats.securedNetworks);
  file.print(F("Hidden networks: "));
  file.println(stats.hiddenNetworks);
  file.print(F("BLE beacons: "));
  file.println(stats.bleBeacons);
  file.print(F("SD failures: "));
  file.println(stats.sdWriteFailures);
  file.print(F("Last error: "));
  file.println(stats.lastError.length() ? stats.lastError : String("none"));
  file.println();
  bool ok = file.getWriteError() == 0;
  file.close();
  return ok;
}

String StorageManager::csvEscape(const String &value) const {
  String escaped = "\"";
  for (uint16_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '"') {
      escaped += "\"\"";
    } else if (c >= 32 && c != 127) {
      escaped += c;
    }
  }
  escaped += "\"";
  return escaped;
}
