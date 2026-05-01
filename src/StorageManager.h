#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <SD.h>

#include "Config.h"
#include "WardriverTypes.h"

class StorageManager {
public:
  bool begin();
  bool ready() const;
  bool ensureWifiLog(const FixData &fix);
  bool writeWifiRecord(const WifiNetworkRecord &record, const FixData &fix);
  bool rotate(const FixData &fix);
  void maybeWriteSummary(const FixData &fix, const AppStats &stats, const AppState &state);
  const String &wifiLogPath() const;
  const String &summaryPath() const;
  uint32_t wifiLogBytes() const;

private:
  bool sdReady = false;
  String activeWifiLog;
  String activeSummary;
  uint32_t rowsSinceFlush = 0;
  uint32_t lastSummaryMs = 0;

  bool createWifiLog(const FixData &fix);
  bool appendHeader(File &file);
  bool appendSummary(const FixData &fix, const AppStats &stats, const AppState &state);
  String csvEscape(const String &value) const;
};

#endif
