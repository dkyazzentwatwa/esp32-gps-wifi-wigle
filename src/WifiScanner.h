#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <Arduino.h>
#include <WiFi.h>

#include "Config.h"
#include "StorageManager.h"
#include "WardriverTypes.h"

class WifiScanner {
public:
  void begin();
  bool ready(uint32_t intervalMs) const;
  void markScanSkipped();
  void forceScan();
  ScanResult scan(const FixData &fix, StorageManager &storage, AppStats &stats);

private:
  String knownBssids[MAX_UNIQUE_BSSIDS];
  uint16_t knownCount = 0;
  uint32_t lastScanMs = 0;

  bool noteUnique(const String &bssid, AppStats &stats);
  void updateTopAps(const WifiNetworkRecord &record, AppStats &stats);
  String authMode(wifi_auth_mode_t auth) const;
  int32_t channelToFrequency(int32_t channel) const;
};

#endif
