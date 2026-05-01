#ifndef SERIAL_SHELL_H
#define SERIAL_SHELL_H

#include <Arduino.h>

#include "BleScanner.h"
#include "DisplayUi.h"
#include "GpsManager.h"
#include "StorageManager.h"
#include "WardriverTypes.h"
#include "WifiScanner.h"

class SerialShell {
public:
  void begin();
  void update(AppState &state, AppStats &stats, GpsManager &gps, StorageManager &storage,
              WifiScanner &wifi, DisplayUi &display, BleScanner &ble);
  void printHelp();

private:
  char buffer[96];
  uint8_t index = 0;

  void handleLine(String line, AppState &state, AppStats &stats, GpsManager &gps,
                  StorageManager &storage, WifiScanner &wifi, DisplayUi &display, BleScanner &ble);
  void printStatus(const AppState &state, const AppStats &stats, GpsManager &gps, StorageManager &storage);
  void printGps(GpsManager &gps);
};

#endif
