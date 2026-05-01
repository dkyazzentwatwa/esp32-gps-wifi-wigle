#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include <Arduino.h>

#include "Config.h"
#include "StorageManager.h"
#include "WardriverTypes.h"

class BleScanner {
public:
  void begin();
  bool available() const;
  void update(const FixData &fix, StorageManager &storage, AppStats &stats);
};

#endif
