#include "GpsManager.h"

GpsManager::GpsManager() : gpsSerial(1) {
}

void GpsManager::begin() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
}

void GpsManager::update() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    lastFixMs = millis();
  }
}

bool GpsManager::gpsDateTimeValid() {
  return gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2024;
}

void GpsManager::syncClock() {
  if (!gpsDateTimeValid()) {
    return;
  }

  DateTime gpsTime(gps.date.year(), gps.date.month(), gps.date.day(),
                   gps.time.hour(), gps.time.minute(), gps.time.second());
  rtc.adjust(gpsTime);
  clockSynced = true;
}

FixData GpsManager::fix() {
  FixData data;
  data.valid = gps.location.isValid() && gps.hdop.isValid() && gps.satellites.isValid();
  data.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
  data.ageMs = gps.location.isValid() ? gps.location.age() : UINT32_MAX;
  data.fresh = data.valid && data.satellites >= MIN_SATELLITES && data.ageMs <= GPS_FIX_STALE_MS;
  data.latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
  data.longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
  data.altitudeMeters = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
  data.hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 99.9;
  data.charsProcessed = gps.charsProcessed();

  DateTime dt = rtc.now();
  formatTimestamp(dt, data.timestamp, sizeof(data.timestamp));
  if (clockSynced) {
    formatFilenameStamp(dt, data.filenameStamp, sizeof(data.filenameStamp));
  } else {
    snprintf(data.filenameStamp, sizeof(data.filenameStamp), "BOOT_%lu", millis() / 1000UL);
  }

  cachedFix = data;
  return data;
}

DateTime GpsManager::now() {
  return rtc.now();
}

void GpsManager::formatTimestamp(const DateTime &dt, char *buffer, size_t len) const {
  snprintf(buffer, len, "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
}

void GpsManager::formatFilenameStamp(const DateTime &dt, char *buffer, size_t len) const {
  snprintf(buffer, len, "%04d%02d%02d_%02d%02d%02d",
           dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
}
