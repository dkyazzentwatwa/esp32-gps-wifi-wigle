#include "BleScanner.h"
#include "Config.h"
#include "DisplayUi.h"
#include "GpsManager.h"
#include "SerialShell.h"
#include "StorageManager.h"
#include "WardriverTypes.h"
#include "WifiScanner.h"

AppStats stats;
AppState appState;
GpsManager gpsManager;
StorageManager storageManager;
WifiScanner wifiScanner;
DisplayUi displayUi;
BleScanner bleScanner;
SerialShell serialShell;

static uint32_t lastButtonReadMs = 0;
static bool lastNextButtonState = HIGH;
static bool lastSelectButtonState = HIGH;

static bool buttonEnabled(int pin) {
  return pin >= 0;
}

static bool buttonPressed(int pin, bool &lastState) {
  if (!buttonEnabled(pin)) {
    return false;
  }

  bool currentState = digitalRead(pin);
  bool pressed = lastState == HIGH && currentState == LOW;
  lastState = currentState;
  return pressed;
}

static void handleButtons() {
  if (millis() - lastButtonReadMs < BUTTON_DEBOUNCE_MS) {
    return;
  }
  lastButtonReadMs = millis();

  if (buttonPressed(BUTTON_NEXT_PIN, lastNextButtonState)) {
    displayUi.nextPage();
  }

  if (buttonPressed(BUTTON_SELECT_PIN, lastSelectButtonState)) {
    appState.loggingEnabled = !appState.loggingEnabled;
    displayUi.flashMessage(appState.loggingEnabled ? "Logging enabled" : "Logging paused");
  }
}

static void setLastError(const String &message) {
  appState.lastError = message;
  stats.lastError = message;
  displayUi.flashMessage(message);
  Serial.print(F("ERROR: "));
  Serial.println(message);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(250);
  Serial.println();
  Serial.println(F("ESP32 Wardriver Pro"));
  stats.startMs = millis();

  if (buttonEnabled(BUTTON_NEXT_PIN)) {
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
  }
  if (buttonEnabled(BUTTON_SELECT_PIN)) {
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  }

  displayUi.begin();
  displayUi.flashMessage("Wardriver Pro");

  gpsManager.begin();
  wifiScanner.begin();
  bleScanner.begin();

  if (!storageManager.begin()) {
    setLastError("SD card missing");
  }

  serialShell.begin();
  serialShell.printHelp();
}

void loop() {
  gpsManager.update();
  gpsManager.syncClock();

  FixData fix = gpsManager.fix();
  handleButtons();
  serialShell.update(appState, stats, gpsManager, storageManager, wifiScanner, displayUi, bleScanner);

  uint32_t intervalMs = appState.fastScan ? FAST_SCAN_INTERVAL_MS : SCAN_INTERVAL_MS;
  bool shouldScan = appState.immediateScan || wifiScanner.ready(intervalMs);
  if (shouldScan) {
    appState.immediateScan = false;

    if (!appState.loggingEnabled) {
      wifiScanner.markScanSkipped();
    } else if (!fix.fresh) {
      stats.gpsWaits++;
      displayUi.flashMessage("Waiting for GPS");
    } else if (!storageManager.ready()) {
      stats.sdWriteFailures++;
      setLastError("SD not ready");
    } else if (!storageManager.ensureWifiLog(fix)) {
      stats.sdWriteFailures++;
      setLastError("Log open failed");
    } else {
      ScanResult result = wifiScanner.scan(fix, storageManager, stats);
      if (!result.ok) {
        setLastError(result.message);
      } else {
        appState.lastError = "";
      }
    }
  }

  if (ENABLE_BLE_SCANNER && appState.bleEnabled && fix.fresh && storageManager.ready()) {
    bleScanner.update(fix, storageManager, stats);
  }

  storageManager.maybeWriteSummary(gpsManager.fix(), stats, appState);
  displayUi.update(fix, stats, appState, storageManager);
}
