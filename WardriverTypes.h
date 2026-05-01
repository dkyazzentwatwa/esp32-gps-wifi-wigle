#ifndef WARDRIVER_TYPES_H
#define WARDRIVER_TYPES_H

#include <Arduino.h>

#include "Config.h"

struct FixData {
  bool valid = false;
  bool fresh = false;
  double latitude = 0.0;
  double longitude = 0.0;
  double altitudeMeters = 0.0;
  double hdop = 99.9;
  uint32_t ageMs = 0;
  uint32_t charsProcessed = 0;
  uint8_t satellites = 0;
  char timestamp[24] = "0000-00-00 00:00:00";
  char filenameStamp[20] = "NOFIX";
};

struct WifiNetworkRecord {
  String bssid;
  String ssid;
  String authMode;
  int32_t rssi = 0;
  int32_t channel = 0;
  int32_t frequencyMHz = 0;
  bool hidden = false;
};

struct TopAccessPoint {
  String bssid;
  String ssid;
  int32_t rssi = -128;
  int32_t channel = 0;
};

struct AppStats {
  uint32_t scans = 0;
  uint32_t skippedScans = 0;
  uint32_t gpsWaits = 0;
  uint32_t networksSeen = 0;
  uint32_t uniqueNetworks = 0;
  uint32_t openNetworks = 0;
  uint32_t securedNetworks = 0;
  uint32_t hiddenNetworks = 0;
  uint32_t bleBeacons = 0;
  uint32_t sdWriteFailures = 0;
  uint32_t rowsWritten = 0;
  uint32_t rotations = 0;
  uint32_t startMs = 0;
  String lastError;
  TopAccessPoint topAps[TOP_AP_COUNT];
};

struct AppState {
  bool loggingEnabled = true;
  bool fastScan = false;
  bool bleEnabled = false;
  bool immediateScan = false;
  String lastError;
};

struct ScanResult {
  bool ok = true;
  String message;
};

#endif
