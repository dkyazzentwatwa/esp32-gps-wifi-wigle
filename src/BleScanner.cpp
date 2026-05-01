#include "BleScanner.h"

#if ENABLE_BLE_SCANNER
#include <BLEDevice.h>

void BleScanner::begin() {
  BLEDevice::init("ESP32WardriverPro");
}

bool BleScanner::available() const {
  return true;
}

void BleScanner::update(const FixData &fix, StorageManager &storage, AppStats &stats) {
  (void)fix;
  (void)storage;
  (void)stats;
  // BLE CSV logging is intentionally separated from WiFi Wigle output.
  // The hook is compiled only when ENABLE_BLE_SCANNER is set to 1.
}

#else

void BleScanner::begin() {
}

bool BleScanner::available() const {
  return false;
}

void BleScanner::update(const FixData &fix, StorageManager &storage, AppStats &stats) {
  (void)fix;
  (void)storage;
  (void)stats;
}

#endif
