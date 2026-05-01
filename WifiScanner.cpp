#include "WifiScanner.h"

void WifiScanner::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);
  lastScanMs = millis() - SCAN_INTERVAL_MS;
}

bool WifiScanner::ready(uint32_t intervalMs) const {
  return millis() - lastScanMs >= intervalMs;
}

void WifiScanner::markScanSkipped() {
  lastScanMs = millis();
}

void WifiScanner::forceScan() {
  lastScanMs = 0;
}

ScanResult WifiScanner::scan(const FixData &fix, StorageManager &storage, AppStats &stats) {
  ScanResult result;
  lastScanMs = millis();

  int networkCount = WiFi.scanNetworks(false, true);
  if (networkCount < 0) {
    result.ok = false;
    result.message = "WiFi scan failed";
    return result;
  }

  stats.scans++;
  stats.networksSeen += networkCount;

  Serial.print(F("Scan #"));
  Serial.print(stats.scans);
  Serial.print(F(" found "));
  Serial.print(networkCount);
  Serial.println(F(" networks"));

  for (int i = 0; i < networkCount; i++) {
    WifiNetworkRecord record;
    record.bssid = WiFi.BSSIDstr(i);
    record.ssid = WiFi.SSID(i);
    record.authMode = authMode(WiFi.encryptionType(i));
    record.rssi = WiFi.RSSI(i);
    record.channel = WiFi.channel(i);
    record.frequencyMHz = channelToFrequency(record.channel);
    record.hidden = record.ssid.length() == 0;

    noteUnique(record.bssid, stats);
    updateTopAps(record, stats);

    if (record.hidden) {
      stats.hiddenNetworks++;
    }
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      stats.openNetworks++;
    } else {
      stats.securedNetworks++;
    }

    if (!storage.writeWifiRecord(record, fix)) {
      stats.sdWriteFailures++;
      result.ok = false;
      result.message = "CSV write failed";
      break;
    }
    stats.rowsWritten++;

    Serial.print(F("  "));
    Serial.print(record.bssid);
    Serial.print(F(" ch "));
    Serial.print(record.channel);
    Serial.print(F(" "));
    Serial.print(record.rssi);
    Serial.print(F(" dBm "));
    Serial.println(record.ssid.length() ? record.ssid : String("<hidden>"));
  }

  WiFi.scanDelete();
  return result;
}

bool WifiScanner::noteUnique(const String &bssid, AppStats &stats) {
  for (uint16_t i = 0; i < knownCount; i++) {
    if (knownBssids[i] == bssid) {
      return false;
    }
  }

  if (knownCount >= MAX_UNIQUE_BSSIDS) {
    return false;
  }

  knownBssids[knownCount++] = bssid;
  stats.uniqueNetworks++;
  return true;
}

void WifiScanner::updateTopAps(const WifiNetworkRecord &record, AppStats &stats) {
  for (uint8_t i = 0; i < TOP_AP_COUNT; i++) {
    if (stats.topAps[i].bssid == record.bssid) {
      stats.topAps[i].ssid = record.ssid;
      stats.topAps[i].rssi = record.rssi;
      stats.topAps[i].channel = record.channel;
      return;
    }
  }

  for (uint8_t i = 0; i < TOP_AP_COUNT; i++) {
    if (record.rssi > stats.topAps[i].rssi) {
      for (int8_t j = TOP_AP_COUNT - 1; j > i; j--) {
        stats.topAps[j] = stats.topAps[j - 1];
      }
      stats.topAps[i].bssid = record.bssid;
      stats.topAps[i].ssid = record.ssid;
      stats.topAps[i].rssi = record.rssi;
      stats.topAps[i].channel = record.channel;
      return;
    }
  }
}

String WifiScanner::authMode(wifi_auth_mode_t auth) const {
  switch (auth) {
    case WIFI_AUTH_OPEN:
      return "OPEN";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2_WPA3_PSK";
    default:
      return "UNKNOWN";
  }
}

int32_t WifiScanner::channelToFrequency(int32_t channel) const {
  if (channel >= 1 && channel <= 13) {
    return 2407 + channel * 5;
  }
  if (channel == 14) {
    return 2484;
  }
  if (channel >= 32 && channel <= 177) {
    return 5000 + channel * 5;
  }
  return 0;
}
