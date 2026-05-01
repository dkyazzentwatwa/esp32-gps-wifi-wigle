#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <RTClib.h>
#include <TinyGPSPlus.h>

#include "Config.h"
#include "WardriverTypes.h"

class GpsManager {
public:
  GpsManager();
  void begin();
  void update();
  void syncClock();
  FixData fix();
  DateTime now();

private:
  TinyGPSPlus gps;
  HardwareSerial gpsSerial;
  RTC_Millis rtc;
  bool clockSynced = false;
  uint32_t lastFixMs = 0;
  FixData cachedFix;

  bool gpsDateTimeValid();
  void formatTimestamp(const DateTime &dt, char *buffer, size_t len) const;
  void formatFilenameStamp(const DateTime &dt, char *buffer, size_t len) const;
};

#endif
